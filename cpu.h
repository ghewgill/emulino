/*
 * Internal header file for emulino
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

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned long u32;
typedef int bool;

#define false   0
#define true    (!false)

typedef u8 (*ReadFunction)(u16 addr);
typedef void (*WriteFunction)(u16 addr, u8 value);
typedef void (*PollFunction)();

void irq(int n);

void register_io(u16 addr, ReadFunction rf, WriteFunction wf);
void register_poll(PollFunction pf);
bool LoadHex(const char *fn, u8 *data, unsigned int size);

#define BIT(b) (1 << (b))

#define PROGRAM_SIZE_WORDS  0x10000
#define DATA_SIZE_BYTES     0x900

void cpu_init();
void cpu_reset();
void cpu_run();
