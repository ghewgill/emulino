/*
 * Port module for emulino
 * Copyright 2009 Greg Hewgill
 *
 * This file is part of Emulino.
 *
 * Emulino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Emulino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emulino.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "port.h"

#include <stdio.h>

#include "cpu.h"

#define PORT_BASE   0x23

#define INDEX_PIN   0
#define INDEX_DDR   1
#define INDEX_PORT  2

u8 PIN[3];
u8 DDR[3];
u8 PORT[3];

inline int port(u16 addr)
{
    return (addr - PORT_BASE) / 3;
}

u8 port_pin_read(u16 addr)
{
    int p = port(addr);
    return (PIN[p] & ~DDR[p]) | (PORT[p] & DDR[p]);
}

void port_pin_write(u16 addr, u8 value)
{
    int p = port(addr);
    PORT[p] ^= value;
}

u8 port_ddr_read(u16 addr)
{
    int p = port(addr);
    return DDR[p];
}

void port_ddr_write(u16 addr, u8 value)
{
    int p = port(addr);
    DDR[p] = value;
}

u8 port_data_read(u16 addr)
{
    int p = port(addr);
    return PORT[p];
}

void port_data_write(u16 addr, u8 value)
{
    int p = port(addr);
    u8 prev = PORT[p];
    PORT[p] = value;
    u8 diff = (prev ^ PORT[p]) & DDR[p];
    int pin = 7;
    u8 bit;
    for (bit = 0x80; bit != 0; bit >>= 1, pin--) {
        if (diff & bit) {
            out_pin(PIN_PORTB+8*p+pin, (PORT[p] & bit) != 0);
        }
    }
}

void port_pin(int pin, bool state)
{
    if (pin >= PIN_PORTB && pin < PIN_PORTB+8) {
        int p = pin - PIN_PORTB;
        PIN[0] = (PIN[0] & ~BIT(p)) | ((state ? 1 : 0) << p);
    }
    if (pin >= PIN_PORTC && pin < PIN_PORTC+8) {
        int p = pin - PIN_PORTC;
        PIN[1] = (PIN[1] & ~BIT(p)) | ((state ? 1 : 0) << p);
    }
    if (pin >= PIN_PORTD && pin < PIN_PORTD+8) {
        int p = pin - PIN_PORTD;
        PIN[2] = (PIN[2] & ~BIT(p)) | ((state ? 1 : 0) << p);
    }
}

void port_init()
{
    int i;
    for (i = 0; i < 3; i++) {
        register_io(PORT_BASE + 3*i + INDEX_PIN, port_pin_read, port_pin_write);
        register_io(PORT_BASE + 3*i + INDEX_DDR, port_ddr_read, port_ddr_write);
        register_io(PORT_BASE + 3*i + INDEX_PORT, port_data_read, port_data_write);
    }
}
