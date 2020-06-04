//==========================================================================
// Name:            tap.h
// Purpose:         create and handle TAP device for data channel
// Date:            June 1, 2020
// Authors:         Jeroen Vreeken
// 
// License:
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 2.1 of the License, or
//  (at your option) any later version.
//
//  distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//  License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//==========================================================================
#ifndef __TAP__
#define __TAP__

#include <stdint.h>
#include <stdlib.h>

struct tap;

struct tap *tap_alloc(const char *dev, uint8_t mac[6]);
void tap_destroy(struct tap *tap);

int tap_rx(struct tap *tap, unsigned char *packet, size_t size);
int tap_tx_check(struct tap *tap);
int tap_tx_get(struct tap *tap, unsigned char *data, size_t *size);

#endif
