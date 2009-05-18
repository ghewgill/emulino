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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

extern u16 Program[];

void LoadBinary(const char *fn)
{
    fprintf(stderr, "emulino: Loading binary image: %s\n", fn);
    FILE *f = fopen(fn, "rb");
    if (f == NULL) {
        perror(fn);
        exit(1);
    }
    fread(Program, 2, PROGRAM_SIZE_WORDS, f);
    fclose(f);
}

bool LoadHex(const char *fn, u8 *data, unsigned int size)
{
    fprintf(stderr, "emulino: Loading hex image: %s\n", fn);
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        return false;
    }
    char buf[100];
    int eof = 0;
    while (!eof && fgets(buf, sizeof(buf), f)) {
        assert(buf[0] == ':');
        u8 c = 0;
        int i;
        for (i = 1; isalnum(buf[i]); i += 2) {
            int x;
            sscanf(buf+i, "%02x", &x);
            c += x;
        }
        assert(c == 0);
        int n, a, t;
        sscanf(buf+1, "%02x%04x%02x", &n, &a, &t);
        switch (t) {
        case 0x00:
            for (i = 0; i < n; i++) {
                int x;
                sscanf(buf+9+i*2, "%02x", &x);
                data[a+i] = x;
            }
            break;
        case 0x01:
            eof = 1;
            break;
        default:
            fprintf(stderr, "unsupported hex type: %02x\n", t);
            exit(1);
        }
    }
    fclose(f);
    return true;
}

void Load(const char *fn)
{
    FILE *f = fopen(fn, "r");
    if (f == 0) {
        perror(fn);
        exit(1);
    }
    char buf[100];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    int n;
    if (sscanf(buf, ":%02x", &n) == 1 && strcspn(buf, "\r\n") == 11+2*n) {
        if (!LoadHex(fn, (u8 *)Program, 2*PROGRAM_SIZE_WORDS)) {
            perror(fn);
            exit(1);
        }
    } else {
        LoadBinary(fn);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s image\n"
                        "       image is a raw binary or hex image file\n", argv[0]);
        exit(1);
    }

    Load(argv[1]);

    cpu_init();
    for (;;) {
        cpu_run();
    }
    return 0;
}
