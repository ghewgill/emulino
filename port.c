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

#include "cpu.h"

#define PORT_PINB   0x23
#define PORT_DDRB   0x24
#define PORT_PORTB  0x25

u8 PINB;
u8 DDRB;
u8 PORTB;

u8 port_pin_read(u16 addr)
{
    return (PINB & ~DDRB) | (PORTB & DDRB);
}

void port_pin_write(u16 addr, u8 value)
{
    PORTB ^= value;
}

u8 port_ddr_read(u16 addr)
{
    return DDRB;
}

void port_ddr_write(u16 addr, u8 value)
{
    DDRB = value;
}

u8 port_data_read(u16 addr)
{
    return PORTB;
}

void port_data_write(u16 addr, u8 value)
{
    PORTB = value;
}

void port_init()
{
    register_io(PORT_PINB, port_pin_read, port_pin_write);
    register_io(PORT_DDRB, port_ddr_read, port_ddr_write);
    register_io(PORT_PORTB, port_data_read, port_data_write);
}
