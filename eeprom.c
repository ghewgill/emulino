/*
 * EEPROM module for emulino
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

#include "eeprom.h"

#include <string.h>

#include "cpu.h"

#define EEPROM_SIZE 1024

#define EEPROM_EECR     0x3f
#define EEPROM_EEDR     0x40
#define EEPROM_EEARL    0x41
#define EEPROM_EEARH    0x42

#define EEPROM_EECR_EERE    BIT(0)
#define EEPROM_EECR_EEPE    BIT(1)
#define EEPROM_EECR_EEMPE   BIT(2)
#define EEPROM_EECR_EERIE   BIT(3)
#define EEPROM_EECR_EEPM0   BIT(4)
#define EEPROM_EECR_EEPM1   BIT(5)

static u8 EEPROM[EEPROM_SIZE];
static u8 EECR;
static u8 EEDR;
static u16 EEAR;

u8 eeprom_read_eecr(u16 addr)
{
    return EECR;
}

void eeprom_write_eecr(u16 addr, u8 value)
{
    EECR = value;
}

u8 eeprom_read_eedr(u16 addr)
{
    return EEPROM[EEAR];
}

void eeprom_write_eedr(u16 addr, u8 value)
{
    EEDR = value;
}

u8 eeprom_read_eearl(u16 addr)
{
    return EEAR & 0xff;
}

void eeprom_write_eearl(u16 addr, u8 value)
{
    EEAR = (EEAR & ~0xff) | value;
}

u8 eeprom_read_eearh(u16 addr)
{
    return (EEAR & (EEPROM_SIZE-1)) >> 8;
}

void eeprom_write_eearh(u16 addr, u8 value)
{
    EEAR = (EEAR & ~0xff00) | ((value << 8) & (EEPROM_SIZE-1));
}

void eeprom_load(u8 *buf, u32 bufsize)
{
    memcpy(EEPROM, buf, bufsize);
}

void eeprom_init()
{
    register_io(EEPROM_EECR, eeprom_read_eecr, eeprom_write_eecr);
    register_io(EEPROM_EEDR, eeprom_read_eedr, eeprom_write_eedr);
    register_io(EEPROM_EEARL, eeprom_read_eearl, eeprom_write_eearl);
    register_io(EEPROM_EEARH, eeprom_read_eearh, eeprom_write_eearh);
}
