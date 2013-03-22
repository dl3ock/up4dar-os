/*

Copyright (C) 2013   Artem Prilutskiy, R3ABM (r3abm@dstar.su)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "aprs.h"

#include "queue.h"
#include "semphr.h"

#include "settings.h"
#include "sw_update.h"
#include "vdisp.h"
#include "rtclock.h"
#include "up_io/eth.h"
#include "up_io/eth_txmem.h"
#include "up_net/ipneigh.h"
#include "up_net/ipv4.h"
#include "up_net/dhcp.h"
#include "up_net/dns_cache.h"
#include "up_net/syslog.h"

#include "up_sys/timer.h"

#define RP2C_POLL_INTERVAL        15
#define RP2C_DATA_TIMEOUT         60

#define RP2C_SIGN_SIZE            4
#define RADIO_HEADER_SIZE         41
#define VOICE_FRAME_SIZE          12

#define RP2C_TYPE_DIGITAL_VOICE   0x20
#define RP2C_TYPE_DIGITAL_DATA    0x40

#define TRUNK_HEADER_SIZE         7

#define RP2C_UDP_PORT             20000
#define RP2C_POLL_INTERVAL        200

#define DSTR_INIT_SIGN            "INIT"
#define DSTR_DATA_SIGN            "DSTR"

#define DSTR_CLASS_SENT           ('s' << 8)
#define DSTR_CLASS_REPLIED        ('r' << 8)
#define DSTR_CLASS_DIGITAL_DATA   (1 << 0)
#define DSTR_CLASS_DIGITAL_VOICE  (1 << 1)
#define DSTR_CLASS_TRANSMISSION   (1 << 4)
#define DSTR_CLASS_LAST_HEARD     (1 << 5)

#define DSTR_TYPE_ACK             (DSTR_CLASS_REPLIED)
#define DSTR_TYPE_POLL            (DSTR_CLASS_SENT)
#define DSTR_TYPE_DIGITAL_DATA    (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_DATA | DSTR_CLASS_TRANSMISSION)
#define DSTR_TYPE_DIGITAL_VOICE   (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_VOICE | DSTR_CLASS_TRANSMISSION)
#define DSTR_TYPE_LAST_HEARD      (DSTR_CLASS_SENT | DSTR_CLASS_DIGITAL_VOICE | DSTR_CLASS_LAST_HEARD)

#define DSTR_FLAG_SENT            (DSTR_CLASS_SENT ^ DSTR_CLASS_REPLIED)

#define DSTR_HEADER_SIZE          10

static unsigned long time1;
static unsigned long time2;
static uint16_t number;

static uint8_t address[4];
static uint16_t port;

void build_dstr_header(uint8_t* buffer, const char* sign, int type, int length)
{
  memcpy(buffer, sign, RP2C_SIGN_SIZE);
  buffer[4] = number >> 8;
  buffer[5] = number & 0xff;
  buffer[6] = type >> 8;
  buffer[7] = type & 0xff;
  buffer[8] = length >> 8;
  buffer[9] = length & 0xff;
}

void build_trunk_header(uint8_t* buffer, const char* type, uint16_t session, uint16_t sequence)
{
  buffer[0] = type;
  memset(buffer + 1, 0, 3);
  buffer[4] = session >> 8;
  buffer[5] = session & 0xff;
  buffer[6] = sequence;
}

void rp2c_confirm_packet(const char* sign)
{
  eth_txmem_t * packet = udp4_get_packet_mem(DSTR_HEADER_SIZE, RP2C_UDP_PORT, port, address);

  if (packet == NULL)
    return;

  uint8_t* data = packet->data + UDP_PAYLOAD_OFFSET;
  build_dstr_header(data, sign, DSTR_TYPE_ACK, 0);
  udp4_calc_chksum_and_send(packet, address);
}

void rp2c_send_heard(const char* call, const char* repeater1)
{
  eth_txmem_t * packet = udp4_get_packet_mem(DSTR_HEADER_SIZE + 2 * CALLSIGN_LENGTH, RP2C_UDP_PORT, port, address);

  if (packet == NULL)
    return;

  uint8_t* data = packet->data + UDP_PAYLOAD_OFFSET;

  build_dstr_header(data, DSTR_DATA_SIGN, DSTR_TYPE_LAST_HEARD, 2 * CALLSIGN_LENGTH);
  data += DSTR_HEADER_SIZE;
  memcpy(data, call, CALLSIGN_LENGTH);
  data += CALLSIGN_LENGTH;
  memcpy(data, repeater1, CALLSIGN_LENGTH);

  udp4_calc_chksum_and_send(packet, address);
}

void rp2c_send_dv_data(uint16_t session, uint16_t sequence, const char* buffer, int length)
{
  eth_txmem_t * packet = udp4_get_packet_mem(DSTR_HEADER_SIZE + TRUNK_HEADER_SIZE + length, RP2C_UDP_PORT, port, address);

  if (packet == NULL)
  return;

  uint8_t* data = packet->data + UDP_PAYLOAD_OFFSET;

  build_dstr_header(data, DSTR_DATA_SIGN, DSTR_TYPE_DIGITAL_DATA, TRUNK_HEADER_SIZE + length);
  data += DSTR_HEADER_SIZE;
  build_trunk_header(data, RP2C_TYPE_DIGITAL_VOICE, session, sequence);
  data += TRUNK_HEADER_SIZE;
  memcpy(data, buffer, length);

  udp4_calc_chksum_and_send(packet, address);
}

void rp2c_send_keepalive()
{
  eth_txmem_t * packet = udp4_get_packet_mem(DSTR_HEADER_SIZE, RP2C_UDP_PORT, port, address);

  if (packet == NULL)
    return;

  uint8_t* data = packet->data + UDP_PAYLOAD_OFFSET;

  build_dstr_header(data, DSTR_DATA_SIGN, DSTR_TYPE_POLL, 0);

  udp4_calc_chksum_and_send(packet, address);
}

void handle_initial_packet(const uint8_t* data, const uint8_t* remote, uint16_t source)
{
  int type = (data[6] << 8) | data[7];

  if ((type == DSTR_TYPE_POLL) && (the_clock > time1))
  {
    port = source;
    memcpy(address, remote, sizeof(address));

    rp2c_confirm_packet(DSTR_INIT_SIGN);

    number = (data[4] << 8) | data[5];
    number ++;

    time1 = the_clock + RP2C_POLL_INTERVAL;
    time2 = the_clock + RP2C_DATA_TIMEOUT;

    syslog(LOG_DAEMON, LOG_INFO, "RP2C: connected to gateway");
  }
}

void hadle_data_packet(const uint8_t* data, const uint8_t* address, uint16_t port)
{
  int type = (data[6] << 8) | data[7];

  if (type & DSTR_FLAG_SENT)
  {
    rp2c_confirm_packet(DSTR_DATA_SIGN);

    number = (data[4] << 8) | data[5];
    number ++;
  }

  switch (type)
  {
    case DSTR_TYPE_ACK:
      time2 = the_clock + RP2C_DATA_TIMEOUT;
      break;

    case DSTR_TYPE_DIGITAL_VOICE:
      // Handle DV data
      break;
  }

}

void handle_packet(const uint8_t* data, int length, const char* address, uint16_t port)
{
  if ((length >= DSTR_HEADER_SIZE) && (memcmp(data, DSTR_INIT_SIGN, RP2C_SIGN_SIZE) == 0))
    handle_initial_packet(data, address, port);

  if ((length >= DSTR_HEADER_SIZE) && (memcmp(data, DSTR_DATA_SIGN, RP2C_SIGN_SIZE) == 0))
    hadle_data_packet(data, address, port);
}

int rp2c_is_connected()
{
  return (time1 != 0);
}

void handle_timer()
{
  if ((the_clock > time1) && (the_clock < time2))
  {
    rp2c_send_keepalive();
    time1 = the_clock + RP2C_POLL_INTERVAL;
  }
  if ((the_clock >= time2) && (time1 != 0))
  {
    syslog(LOG_DAEMON, LOG_ERR, "RP2C: connection timed out");
    time1 = 0;
  }
}

void rp2c_init()
{
  udp4_set_socket(UDP_SOCKET_RP2C, RP2C_UDP_PORT, handle_packet);
  timer_set_slot(TIMER_SLOT_RP2C, 1000, handle_timer);
}
