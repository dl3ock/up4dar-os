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

#ifndef RP2C_H
#define RO2C_H

#include "FreeRTOS.h"
#include "gcc_builtin.h"

#define RP2C_NUMBER_LAST_FRAME    0x40
#define RP2C_NUMBER_RADIO_HEADER  0x80
#define RP2C_NUMBER_DIGITAL_DATA  0xC0
#define RP2C_NUMBER_MASK          0x3f

int rp2c_is_connected();

void rp2c_send_heard(const char* call, const char* repeater1);
void rp2c_send_dv_data(uint16_t session, uint16_t sequence, const char* buffer, int length);
// Note: Gateway software determines type of DV data (radio header of frame) by the sequence number and length

void rp2c_init();

#endif