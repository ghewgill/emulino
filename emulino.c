/*
 * emulino - arduino emulator
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

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "loader.h"

bool pins[PIN_COUNT];

void pinchange(int pin, bool state)
{
    fprintf(stderr, "pin %d %d\n", pin, state);
    pins[pin] = state;
    fprintf(stderr, "pins %d%d%d%d%d%d%d%d %d%d%d%d%d%d%d%d %d%d%d%d%d%d%d%d\n",
        pins[ 0], pins[ 1], pins[ 2], pins[ 3], pins[ 4], pins[ 5], pins[ 6], pins[ 7],
        pins[ 8], pins[ 9], pins[10], pins[11], pins[12], pins[13], pins[14], pins[15],
        pins[16], pins[17], pins[18], pins[19], pins[20], pins[21], pins[22], pins[23]);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s image\n"
                        "       image is a raw binary or hex image file\n", argv[0]);
        exit(1);
    }

    u8 prog[PROGRAM_SIZE_WORDS*2];
    u32 progsize = load_file(argv[1], prog, sizeof(prog));
    if (progsize == 0) {
        perror(argv[1]);
        exit(1);
    }

    u8 eeprom[512];
    u32 eepromsize = load_file("emulino.eeprom", eeprom, sizeof(eeprom));

    cpu_init();
    cpu_load_flash(prog, progsize);
    cpu_load_eeprom(eeprom, eepromsize);
    int i;
    for (i = 0; i < 8; i++) {
        cpu_pin_callback(PIN_PORTB+i, pinchange);
        cpu_pin_callback(PIN_PORTC+i, pinchange);
        cpu_pin_callback(PIN_PORTD+i, pinchange);
    }
    for (;;) {
        if (cpu_run() == CPU_HALT) {
            break;
        }
    }
    fprintf(stderr, "cycles: %lu\n", cpu_get_cycles());
    return 0;
}
