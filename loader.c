/*
 * binary file loading utilities
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

#include "loader.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u32 load_binary(const char *fn, u8 *buf, u32 bufsize)
{
    fprintf(stderr, "emulino: Loading binary image: %s\n", fn);
    FILE *f = fopen(fn, "rb");
    if (f == NULL) {
        perror(fn);
        exit(1);
    }
    u32 r = fread(buf, 1, bufsize, f);
    fclose(f);
    return r;
}

u32 load_hex(const char *fn, u8 *buf, u32 bufsize)
{
    fprintf(stderr, "emulino: Loading hex image: %s\n", fn);
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        return 0;
    }
    char s[100];
    int eof = 0;
    u32 m = 0;
    while (!eof && fgets(s, sizeof(s), f)) {
        assert(s[0] == ':');
        u8 c = 0;
        int i;
        for (i = 1; isalnum(s[i]); i += 2) {
            int x;
            sscanf(s+i, "%02x", &x);
            c += x;
        }
        assert(c == 0);
        int n, a, t;
        sscanf(s+1, "%02x%04x%02x", &n, &a, &t);
        switch (t) {
        case 0x00:
            for (i = 0; i < n; i++) {
                int x;
                sscanf(s+9+i*2, "%02x", &x);
                buf[a+i] = x;
                if (a+i >= m) {
                    m = a+i+1;
                }
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
    return m;
}

u32 load_file(const char *fn, u8 *buf, u32 bufsize)
{
    FILE *f = fopen(fn, "r");
    if (f == 0) {
        return 0;
    }
    char s[100];
    fgets(s, sizeof(s), f);
    fclose(f);
    u32 r = 0;
    int n;
    if (sscanf(s, ":%02x", &n) == 1 && strcspn(s, "\r\n") == 11+2*n) {
        r = load_hex(fn, buf, bufsize);
        if (r == 0) {
            perror(fn);
            exit(1);
        }
    } else {
        r = load_binary(fn, buf, bufsize);
    }
    return r;
}
