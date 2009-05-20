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

void pinchange(int pin, bool state)
{
    fprintf(stderr, "pin %d %d\n", pin, state);
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
        cpu_run();
    }
    return 0;
}
