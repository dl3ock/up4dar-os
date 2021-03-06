
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

#ifndef SLOW_DATA_H
#define SLOW_DATA_H

#include "FreeRTOS.h"

uint8_t get_slow_data_chunk(uint8_t* data);
void get_slow_data_block(uint8_t* data, uint8_t frame, size_t duration);
void build_slow_data(uint8_t* data, char last, uint8_t frame, size_t duration);

#endif