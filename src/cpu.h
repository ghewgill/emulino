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

#include "util.h"

#define PROGRAM_SIZE_WORDS  0x10000
#define DATA_SIZE_BYTES     0x900

#define PIN_PORTB   0
#define PIN_PORTC   8
#define PIN_PORTD   16
#define PIN_COUNT   (PIN_PORTD + 8)

#define CPU_RUN     0
#define CPU_HALT    1

typedef u8 (*ReadFunction)(u16 addr);
typedef void (*WriteFunction)(u16 addr, u8 value);
typedef void (*PollFunction)();
typedef void (*PinFunction)(int pin, bool state);

#ifdef __cplusplus
extern "C" {
#endif

void irq(int n);

void register_io(u16 addr, ReadFunction rf, WriteFunction wf);
void register_poll(PollFunction pf);
void out_pin(int pin, bool state);

void cpu_init();
void cpu_load_flash(u8 *buf, u32 bufsize);
void cpu_load_eeprom(u8 *buf, u32 bufsize);
void cpu_usart_set_output(int fd);
void cpu_usart_set_input(int fd);
void cpu_reset();
int cpu_run();
void cpu_set_pin(int pin, bool state);
void cpu_pin_callback(int pin, PinFunction f);
u32 cpu_get_cycles();

#ifdef __cplusplus
} // extern "C"
#endif
