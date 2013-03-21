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
#include "rx_dstar_crc_header.h"
#include "vdisp.h"
#include "rtclock.h"
#include "up_io/eth.h"
#include "up_io/eth_txmem.h"
#include "up_net/ipneigh.h"
#include "up_net/ipv4.h"
#include "up_net/dhcp.h"
#include "up_net/dns_cache.h"

#include "up_sys/timer.h"

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

#define DSTR_HEADER_SIZE          10

static int timeout1;
static int timeout2;
static uint16_t number;

static uint8_t address[4];
static uint16_t port;

void build_dstr_header(uint8_t* buffer, const char* sign, int type, int length)
{
  memcpy(buffer, sign, 4);
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

void rp2c_send_heard(const char* call, const char* repeater1)
{
  eth_txmem_t * packet = udp4_get_packet_mem(DSTR_HEADER_SIZE + 2 * CALLSIGN_LENGTH, RP2C_UDP_PORT, port, address);

  if (packet == NULL)
    return;

  uint8_t* data = packet->data + UDP_PAYLOAD_OFFSET;

  build_dstr_header(data, DSTR_DATA_SIGN, DSTR_TYPE_LAST_HEARD, 0);
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

void rp2c_handle_packet(const uint8_t* data, int length, const char* address, uint16_t port)
{
  
}

void rp2c_handle_timer()
{
  
}

int rp2c_is_connected()
{
  return 0;
}

void rp2c_init()
{
  udp4_set_socket(UDP_SOCKET_RP2C, RP2C_UDP_PORT, rp2c_handle_packet);
  timer_set_slot(TIMER_SLOT_RP2C, RP2C_POLL_INTERVAL, rp2c_handle_timer);
}
