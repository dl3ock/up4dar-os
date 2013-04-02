/*

Copyright (C) 2013   Michael Dirska, DL1BFF (dl1bff@mdx.de)

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

#include "slow_data.h"
#include "settings.h"
#include "gps.h"
#include "aprs.h"

static uint8_t chunk[6];

uint8_t get_slow_data_chunk(uint8_t* data)
{
  int size = 0;

  switch (SETTING_CHAR(C_DPRS_ENABLED))
  {
    case 1:
      size = gps_get_slow_data(data);
      break;

    case 2:
      size = aprs_get_slow_data(data);
      break;
  }

  for (int index = size; index < 5; index ++)
    data[index] = 0x66;

  return size;
}

void get_slow_data_block(uint8_t* data, uint8_t frame, size_t duration)
{
  int parity = frame & 1;

  if (parity)
  {
    if ((frame <= 8) && ((duration % 1260) <= 40))  // send tx_msg twice every 25 sec
    {
      int part = (frame - 1) >> 1;
      memcpy(chunk + 1, settings.s.txmsg + part * 5, 5);
      chunk[0] = 0x40 | part;
    }
    else
    {
      int size = get_slow_data_chunk(chunk + 1);
      chunk[0] = size ? (0x30 | size) : 0x66;
    }
  }

  int index = (parity ^ 1) * 3;
  memcpy(data, chunk + index, 3);
}

void build_slow_data(uint8_t* data, char last, uint8_t frame, size_t duration)
{
  if ((last != 0) || (frame & 0x40))
  {
    data[0] = 0x55;
    data[1] = 0x55;
    data[2] = 0x55;
    return;
  }

  if (frame == 0)
  {
    data[0] = 0x55;
    data[1] = 0x2d;
    data[2] = 0x16;
    return;
  }

  get_slow_data_block(data, frame, duration);

  data[0] ^= 0x70;
  data[1] ^= 0x4f;
  data[2] ^= 0x93;
}
