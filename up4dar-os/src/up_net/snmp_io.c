/*

Copyright (C) 2012   Michael Dirska, DL1BFF (dl1bff@mdx.de)

Copyright (C) 2013   Artem Prilutskiy, R3ABM (r3abm@dstar.su)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "snmp_io.h"

#include "FreeRTOS.h"
#include "gcc_builtin.h"

#include "up_io/eth.h"
#include "up_io/eth_txmem.h"
#include "ipneigh.h"
#include "ipv4.h"

#include "snmp.h"

void snmp_handle_packet(const uint8_t* data, int len, const uint8_t * address, uint16_t src_port, uint16_t dest_port)
{
  int data_length = 0;
  
  eth_txmem_t * packet = snmp_process_request(data, len, & data_length );
  
  if (packet == NULL)  // something went wrong
    return;
  
  if (data_length <= 0)  // error occurred
  {
    eth_txmem_free(packet);
    return;
  }
  
  ipv4_udp_prepare_packet(packet, address, data_length, src_port, dest_port);
  udp4_calc_chksum_and_send(packet,  address);
}

void snmp_io_init()
{
  udp4_set_socket(UDP_SOCKET_SNMP, 161, snmp_handle_packet);
}