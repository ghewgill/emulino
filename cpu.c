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

#include "cpu.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "eeprom.h"
#include "port.h"
#include "timer.h"
#include "usart.h"

//#define TRACE

typedef void (*Handler)(u16 instr);

#define MAX_POLL_FUNCTIONS  16
#define MAX_IRQ             27

typedef struct {
    union {
        struct {
            // 0x00 - 0x1f
            union {
                u8 Reg[32];
                struct {
                    u8 dummy1[24];
                    union {
                        u16 RegW[4];
                        struct {
                            u16 r24;
                            u16 X;
                            u16 Y;
                            u16 Z;
                        };
                    };
                };
            };
            u8 dummy2[0x5d-0x20]; // 0x20 - 0x5c
            u16 SP __attribute__((packed)); // 0x5d - 0x5e
            // 0x5f
            union {
                struct {
                    char C: 1;
                    char Z: 1;
                    char N: 1;
                    char V: 1;
                    char S: 1;
                    char H: 1;
                    char T: 1;
                    char I: 1;
                };
                u8 bits;
            } SREG;
        };
        u8 _Bytes[DATA_SIZE_BYTES];
        //u16 _Words[DATA_SIZE_BYTES/2];
    };
} TData;

static u8 ioread(u16 addr);
static void iowrite(u16 addr, u8 value);

u16 Program[PROGRAM_SIZE_WORDS];
TData Data;
ReadFunction IORead[0x100];
WriteFunction IOWrite[0x100];
PollFunction PollFunctions[MAX_POLL_FUNCTIONS];
int PollFunctionCount;
int PendingIRQ[MAX_IRQ];
int PendingIRQCount;
int State;
u16 PC;
u32 Cycle;
u32 LastPoll;
PinFunction PinCallback[PIN_COUNT];

COMPILE_ASSERT(sizeof(Data.SREG) == 1);
COMPILE_ASSERT(((u8 *)&Data.SP) - Data._Bytes == 0x5d);
COMPILE_ASSERT(((u8 *)&Data.SREG) - Data._Bytes == 0x5f);

static u8 read(u16 addr)
{
    if ((addr & 0xff00) == 0) {
        return ioread(addr);
    } else {
        return Data._Bytes[addr];
    }
}

static void write(u16 addr, u8 value)
{
    if ((addr & 0xff00) == 0) {
        iowrite(addr, value);
    } else {
        Data._Bytes[addr] = value;
    }
}

static int doubleWordInstruction(u16 instr)
{
    return (instr & 0xfe0e) == 0x940e // CALL
        || (instr & 0xfe0e) == 0x940c // JMP
        || (instr & 0xfe0f) == 0x9000 // LDS
        || (instr & 0xfe0f) == 0x9200; // STS
}

static void trace(const char *s)
{
    #ifdef TRACE
        fprintf(stderr, "%s\n", s);
    #endif
}

static void unimplemented(const char *s)
{
    fprintf(stderr, "unimplemented: %s\n", s);
    exit(1);
}

static void do_ADC(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] + Data.Reg[r] + (Data.SREG.C ? 1 : 0);
    Data.SREG.H = (((Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & ~x) | (~x & Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & Data.Reg[r] & ~x) | (~Data.Reg[d] & ~Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & ~x) | (~x & Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_ADD(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] + Data.Reg[r];
    Data.SREG.H = (((Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & ~x) | (~x & Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & Data.Reg[r] & ~x) | (~Data.Reg[d] & ~Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & ~x) | (~x & Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_ADIW(u16 instr)
{
    trace(__FUNCTION__);
    // --------KKddKKKK
    u16 K = (instr & 0xf) | ((instr >> 2) & 0x30);
    u16 d = ((instr >> 4) & 0x3);
    u16 x = Data.RegW[d] + K;
    Data.SREG.V = ((~Data.RegW[d] & x) & 0x8000) != 0;
    Data.SREG.N = (x & 0x8000) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = ((~x & Data.RegW[d]) & 0x8000) != 0;
    Data.RegW[d] = x;
    Cycle += 2;
}

static void do_AND(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] &= Data.Reg[r];
    Data.SREG.V = 0;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.S = Data.SREG.N;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_ANDI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    u8 x = Data.Reg[d] &= K;
    Data.SREG.S = (x & 0x80) != 0;
    Data.SREG.V = 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.Z = x == 0;
    Cycle++;
}

static void do_ASR(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.SREG.C = Data.Reg[d] & 0x01;
    Data.Reg[d] = (s8)Data.Reg[d] >> 1;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.V = Data.SREG.N ^ Data.SREG.C;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_BCLR(u16 instr)
{
    trace(__FUNCTION__);
    // ---------sss----
    u16 s = ((instr >> 4) & 0x7);
    Data.SREG.bits &= ~(1 << s);
    Cycle++;
}

static void do_BLD(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd-bbb
    u16 d = ((instr >> 4) & 0x1f);
    u16 b = (instr & 0x7);
    Data.Reg[d] = (Data.Reg[d] & ~(1 << b)) | ((Data.SREG.T ? 1 : 0) << b);
    Cycle++;
}

static void do_BRBC(u16 instr)
{
    trace(__FUNCTION__);
    // ------kkkkkkksss
    u16 k = ((instr >> 3) & 0x7f);
    u16 s = (instr & 0x7);
    if ((Data.SREG.bits & (1 << s)) == 0) {
        PC += (s8)(k << 1) >> 1;
        Cycle++;
    }
    Cycle++;
}

static void do_BRBS(u16 instr)
{
    trace(__FUNCTION__);
    // ------kkkkkkksss
    u16 k = ((instr >> 3) & 0x7f);
    u16 s = (instr & 0x7);
    if (Data.SREG.bits & (1 << s)) {
        PC += (s8)(k << 1) >> 1;
        Cycle++;
    }
    Cycle++;
}

static void do_BREAK(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
    Cycle++;
}

static void do_BSET(u16 instr)
{
    trace(__FUNCTION__);
    // ---------sss----
    u16 s = ((instr >> 4) & 0x7);
    Data.SREG.bits |= 1 << s;
    Cycle++;
}

static void do_BST(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd-bbb
    u16 d = ((instr >> 4) & 0x1f);
    u16 b = (instr & 0x7);
    Data.SREG.T = ((Data.Reg[d] & (1 << b)) != 0);
    Cycle++;
}

static void do_CALL(u16 instr)
{
    trace(__FUNCTION__);
    // -------kkkkk---k
    u16 k = (instr & 0x1) | ((instr >> 3) & 0x3e);
    k = k << 16 | Program[PC++];
    write(Data.SP--, PC >> 8);
    write(Data.SP--, PC & 0xff);
    PC = k;
    Cycle += 4;
}

static void do_CBI(u16 instr)
{
    trace(__FUNCTION__);
    // --------AAAAAbbb
    u16 A = ((instr >> 3) & 0x1f);
    u16 b = (instr & 0x7);
    write(0x20+A, read(0x20+A) & ~(1 << b));
    Cycle += 2;
}

static void do_COM(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = ~Data.Reg[d];
    Data.SREG.V = 0;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.S = Data.SREG.N;
    Data.SREG.Z = Data.Reg[d] == 0;
    Data.SREG.C = 1;
    Cycle++;
}

static void do_CP(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] - Data.Reg[r];
    Data.SREG.H = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~Data.Reg[r] & ~x) | (~Data.Reg[d] & Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Cycle++;
}

static void do_CPC(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] - Data.Reg[r] - (Data.SREG.C ? 1 : 0);
    Data.SREG.H = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~Data.Reg[r] & ~x) | (~Data.Reg[d] & Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z &= x == 0;
    Data.SREG.C = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Cycle++;
}

static void do_CPI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    u8 x = Data.Reg[d] - K;
    Data.SREG.H = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~K & ~x) | (~Data.Reg[d] & K & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Cycle++;
}

static void do_CPSE(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    if (Data.Reg[d] == Data.Reg[r]) {
        if (doubleWordInstruction(Program[PC])) {
            PC++;
            Cycle++;
        }
        PC++;
        Cycle++;
    }
    Cycle++;
}

static void do_DEC(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d]--;
    Data.SREG.V = Data.Reg[d] == 0x7f;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_DES(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_EICALL(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_EIJMP(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_ELPM_1(u16 instr)
{
    trace(__FUNCTION__);
    Data.Reg[0] = ((u8 *)Program)[Data.Z];
    Cycle += 3;
}

static void do_ELPM_2(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_ELPM_3(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_EOR(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] ^= Data.Reg[r];
    Data.SREG.S = (x & 0x80) != 0;
    Data.SREG.V = 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.Z = x == 0;
    Cycle++;
}

static void do_FMUL(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_FMULS(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_FMULSU(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
}

static void do_ICALL(u16 instr)
{
    trace(__FUNCTION__);
    write(Data.SP--, PC >> 8);
    write(Data.SP--, PC & 0xff);
    PC = Data.Z;
    Cycle += 3;
}

static void do_IJMP(u16 instr)
{
    trace(__FUNCTION__);
    PC = Data.Z;
    Cycle += 2;
}

static void do_IN(u16 instr)
{
    trace(__FUNCTION__);
    // -----AAdddddAAAA
    u16 A = (instr & 0xf) | ((instr >> 5) & 0x30);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(0x20 + A);
    Cycle++;
}

static void do_INC(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d]++;
    Data.SREG.V = Data.Reg[d] == 0x80;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_JMP(u16 instr)
{
    trace(__FUNCTION__);
    // -------kkkkk---k
    u16 k = (instr & 0x1) | ((instr >> 3) & 0x3e);
    k = k << 16 | Program[PC++];
    PC = k;
    Cycle += 3;
}

static void do_LD_X1(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.X);
    Cycle += 2;
}

static void do_LD_X2(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.X++);
    Cycle += 2;
}

static void do_LD_X3(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(--Data.X);
    Cycle += 2;
}

static void do_LD_Y2(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.Y++);
    Cycle += 2;
}

static void do_LD_Y3(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(--Data.Y);
    Cycle += 2;
}

static void do_LD_Y4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-ddddd-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.Y+q);
    Cycle += 2;
}

static void do_LD_Z2(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.Z++);
    Cycle += 2;
}

static void do_LD_Z3(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(--Data.Z);
    Cycle += 2;
}

static void do_LD_Z4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-ddddd-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Data.Z+q);
    Cycle += 2;
}

static void do_LDI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    Data.Reg[d] = K;
    Cycle++;
}

static void do_LDS(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(Program[PC++]);
    Cycle += 2;
}

static void do_LPM_1(u16 instr)
{
    trace(__FUNCTION__);
    Data.Reg[0] = ((u8 *)Program)[Data.Z];
    Cycle += 3;
}

static void do_LPM_2(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = ((u8 *)Program)[Data.Z];
    Cycle += 3;
}

static void do_LPM_3(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = ((u8 *)Program)[Data.Z++];
    Cycle += 3;
}

static void do_LSR(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.SREG.C = Data.Reg[d] & 0x01;
    Data.Reg[d] >>= 1;
    Data.SREG.N = 0;
    Data.SREG.V = Data.SREG.N ^ Data.SREG.C;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_MOV(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = Data.Reg[r];
    Cycle++;
}

static void do_MOVW(u16 instr)
{
    trace(__FUNCTION__);
    // --------ddddrrrr
    u16 d = ((instr >> 4) & 0xf);
    u16 r = (instr & 0xf);
    Data.Reg[d*2] = Data.Reg[r*2];
    Data.Reg[d*2+1] = Data.Reg[r*2+1];
    Cycle++;
}

static void do_MUL(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u16 x = Data.Reg[d] * Data.Reg[r];
    Data.Reg[1] = x >> 8;
    Data.Reg[0] = x & 0xff;
    Data.SREG.C = (x & 0x8000) != 0;
    Data.SREG.Z = x == 0;
    Cycle += 2;
}

static void do_MULS(u16 instr)
{
    trace(__FUNCTION__);
    // --------ddddrrrr
    u16 d = ((instr >> 4) & 0xf);
    u16 r = (instr & 0xf);
    s16 x = (s8)Data.Reg[16+d] * (s8)Data.Reg[16+r];
    Data.Reg[1] = x >> 8;
    Data.Reg[0] = x & 0xff;
    Data.SREG.C = (x & 0x8000) != 0;
    Data.SREG.Z = x == 0;
    Cycle += 2;
}

static void do_MULSU(u16 instr)
{
    trace(__FUNCTION__);
    // ---------ddd-rrr
    u16 d = ((instr >> 4) & 0x7);
    u16 r = (instr & 0x7);
    s16 x = (s8)Data.Reg[16+d] * Data.Reg[16+r];
    Data.Reg[1] = x >> 8;
    Data.Reg[0] = x & 0xff;
    Data.SREG.C = (x & 0x8000) != 0;
    Data.SREG.Z = x == 0;
    Cycle += 2;
}

static void do_NEG(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = -Data.Reg[d];
    Data.SREG.H = ((x | Data.Reg[d]) & 0x08) != 0;
    Data.SREG.V = x == 0x80;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = x != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_NOP(u16 instr)
{
    trace(__FUNCTION__);
    Cycle++;
}

static void do_OR(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] |= Data.Reg[r];
    Data.SREG.V = 0;
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.S = Data.SREG.N;
    Data.SREG.Z = Data.Reg[d] == 0;
    Cycle++;
}

static void do_ORI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    u8 x = Data.Reg[d] |= K;
    Data.SREG.S = (x & 0x80) != 0;
    Data.SREG.V = 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.Z = x == 0;
    Cycle++;
}

static void do_OUT(u16 instr)
{
    trace(__FUNCTION__);
    // -----AArrrrrAAAA
    u16 A = (instr & 0xf) | ((instr >> 5) & 0x30);
    u16 r = ((instr >> 4) & 0x1f);
    write(0x20 + A, Data.Reg[r]);
    Cycle++;
}

static void do_POP(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = read(++Data.SP);
    Cycle += 2;
}

static void do_PUSH(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.SP--, Data.Reg[r]);
    Cycle += 2;
}

static void do_RCALL(u16 instr)
{
    trace(__FUNCTION__);
    // ----kkkkkkkkkkkk
    u16 k = (instr & 0xfff);
    write(Data.SP--, PC >> 8);
    write(Data.SP--, PC & 0xff);
    PC += (s16)(k << 4) >> 4;
    Cycle += 3;
}

static void do_RET(u16 instr)
{
    trace(__FUNCTION__);
    PC = read(Data.SP+1) | (read(Data.SP+2) << 8);
    Data.SP += 2;
    Cycle += 4;
}

static void do_RETI(u16 instr)
{
    trace(__FUNCTION__);
    PC = read(Data.SP+1) | (read(Data.SP+2) << 8);
    Data.SP += 2;
    Data.SREG.I = 1;
    Cycle += 4;
}

static void do_RJMP(u16 instr)
{
    trace(__FUNCTION__);
    // ----kkkkkkkkkkkk
    u16 k = (instr & 0xfff);
    PC += (s16)(k << 4) >> 4;
    Cycle += 2;
}

static void do_ROR(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    int c = Data.Reg[d] & 0x01;
    Data.Reg[d] = (Data.Reg[d] >> 1) | (Data.SREG.C ? 0x80 : 0);
    Data.SREG.N = (Data.Reg[d] & 0x80) != 0;
    Data.SREG.V = Data.SREG.N ^ Data.SREG.C;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = Data.Reg[d] == 0;
    Data.SREG.C = c;
    Cycle++;
}

static void do_SBC(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] - Data.Reg[r] - (Data.SREG.C ? 1 : 0);
    Data.SREG.H = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~Data.Reg[r] & ~x) | (~Data.Reg[d] & Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z &= x == 0;
    Data.SREG.C = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_SBCI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    u8 x = Data.Reg[d] - K - (Data.SREG.C ? 1 : 0);
    Data.SREG.H = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~K & ~x) | (~Data.Reg[d] & K & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z &= x == 0;
    Data.SREG.C = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_SBI(u16 instr)
{
    trace(__FUNCTION__);
    // --------AAAAAbbb
    u16 A = ((instr >> 3) & 0x1f);
    u16 b = (instr & 0x7);
    write(0x20+A, read(0x20+A) | (1 << b));
    Cycle += 2;
}

static void do_SBIC(u16 instr)
{
    trace(__FUNCTION__);
    // --------AAAAAbbb
    u16 A = ((instr >> 3) & 0x1f);
    u16 b = (instr & 0x7);
    if ((read(0x20+A) & (1 << b)) == 0) {
        if (doubleWordInstruction(Program[PC])) {
            PC++;
            Cycle++;
        }
        PC++;
        Cycle++;
    }
    Cycle++;
}

static void do_SBIS(u16 instr)
{
    trace(__FUNCTION__);
    // --------AAAAAbbb
    u16 A = ((instr >> 3) & 0x1f);
    u16 b = (instr & 0x7);
    if ((read(0x20+A) & (1 << b)) != 0) {
        if (doubleWordInstruction(Program[PC])) {
            PC++;
            Cycle++;
        }
        PC++;
        Cycle++;
    }
    Cycle++;
}

static void do_SBIW(u16 instr)
{
    trace(__FUNCTION__);
    // --------KKddKKKK
    u16 K = (instr & 0xf) | ((instr >> 2) & 0x30);
    u16 d = ((instr >> 4) & 0x3);
    u16 x = Data.RegW[d] - K;
    Data.SREG.V = ((Data.RegW[d] & ~x) & 0x8000) != 0;
    Data.SREG.N = (x & 0x8000) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = ((x & ~Data.RegW[d]) & 0x8000) != 0;
    Data.RegW[d] = x;
    Cycle += 2;
}

static void do_SBRC(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr-bbb
    u16 r = ((instr >> 4) & 0x1f);
    u16 b = (instr & 0x7);
    if ((Data.Reg[r] & (1 << b)) == 0) {
        if (doubleWordInstruction(Program[PC])) {
            PC++;
            Cycle++;
        }
        PC++;
        Cycle++;
    }
    Cycle++;
}

static void do_SBRS(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr-bbb
    u16 r = ((instr >> 4) & 0x1f);
    u16 b = (instr & 0x7);
    if (Data.Reg[r] & (1 << b)) {
        if (doubleWordInstruction(Program[PC])) {
            PC++;
            Cycle++;
        }
        PC++;
        Cycle++;
    }
    Cycle++;
}

static void do_SLEEP(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
    Cycle++;
}

static void do_SPM2_1(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
    // unknown cycles
}

static void do_SPM2_2(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
    // unknown cycles
}

static void do_ST_X1(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.X, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_X2(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.X++, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_X3(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(--Data.X, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Y2(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.Y++, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Y3(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(--Data.Y, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Y4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-rrrrr-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.Y+q, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Z2(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.Z++, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Z3(u16 instr)
{
    trace(__FUNCTION__);
    // -------rrrrr----
    u16 r = ((instr >> 4) & 0x1f);
    write(--Data.Z, Data.Reg[r]);
    Cycle += 2;
}

static void do_ST_Z4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-rrrrr-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 r = ((instr >> 4) & 0x1f);
    write(Data.Z+q, Data.Reg[r]);
    Cycle += 2;
}

static void do_STS(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    write(Program[PC++], Data.Reg[d]);
    Cycle += 2;
}

static void do_SUB(u16 instr)
{
    trace(__FUNCTION__);
    // ------rdddddrrrr
    u16 r = (instr & 0xf) | ((instr >> 5) & 0x10);
    u16 d = ((instr >> 4) & 0x1f);
    u8 x = Data.Reg[d] - Data.Reg[r];
    Data.SREG.H = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~Data.Reg[r] & ~x) | (~Data.Reg[d] & Data.Reg[r] & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_SUBI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    u8 x = Data.Reg[d] - K;
    Data.SREG.H = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x08) != 0;
    Data.SREG.V = (((Data.Reg[d] & ~K & ~x) | (~Data.Reg[d] & K & x)) & 0x80) != 0;
    Data.SREG.N = (x & 0x80) != 0;
    Data.SREG.S = Data.SREG.N ^ Data.SREG.V;
    Data.SREG.Z = x == 0;
    Data.SREG.C = (((~Data.Reg[d] & K) | (K & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
    Data.Reg[d] = x;
    Cycle++;
}

static void do_SWAP(u16 instr)
{
    trace(__FUNCTION__);
    // -------ddddd----
    u16 d = ((instr >> 4) & 0x1f);
    Data.Reg[d] = (Data.Reg[d] << 4) | (Data.Reg[d] >> 4);
    Cycle++;
}

static void do_WDR(u16 instr)
{
    trace(__FUNCTION__);
    unimplemented(__FUNCTION__);
    Cycle++;
}

static void do_halt(u16 instr)
{
    State = CPU_HALT;
}

#include "avr.inc"

void irq(int n)
{
    if (Data.SREG.I) {
        #ifdef TRACE
            if (n != 17) { // timer
                fprintf(stderr, "irq: %d\n", n);
            }
        #endif
        write(Data.SP--, PC >> 8);
        write(Data.SP--, PC & 0xff);
        PC = (n - 1) << 1;
        Data.SREG.I = 0;
    } else {
        PendingIRQ[n] = 1;
        PendingIRQCount++;
    }
}

static u8 ioread(u16 addr)
{
    //fprintf(stderr, "ioread %04x\n", addr);
    ReadFunction f = IORead[addr];
    if (f != NULL) {
        return f(addr);
    }
    return Data._Bytes[addr];
}

static void iowrite(u16 addr, u8 value)
{
    if (addr != 0x5f) {
        #ifdef TRACE
            fprintf(stderr, "iowrite %04x %02x\n", addr, value);
        #endif
    }
    WriteFunction f = IOWrite[addr];
    if (f != NULL) {
        f(addr, value);
    }
    Data._Bytes[addr] = value;
    if (addr == 0x5f && PendingIRQCount > 0 && Data.SREG.I) {
        int i;
        for (i = 0; i < MAX_IRQ; i++) {
            if (PendingIRQ[i]) {
                #ifdef TRACE
                    fprintf(stderr, "pending irq %d\n", i);
                #endif
                PendingIRQ[i] = 0;
                PendingIRQCount--;
                irq(i);
                break;
            }
        }
    }
}

void register_io(u16 addr, ReadFunction rf, WriteFunction wf)
{
    assert(IORead[addr] == NULL);
    IORead[addr] = rf;

    assert(IOWrite[addr] == NULL);
    IOWrite[addr] = wf;
}

void register_poll(PollFunction pf)
{
    PollFunctions[PollFunctionCount++] = pf;
}

void out_pin(int pin, bool state)
{
    if (PinCallback[pin] != NULL) {
        PinCallback[pin](pin, state);
    }
}

void cpu_init()
{
    eeprom_init();
    port_init();
    timer_init();
    usart_init();

    // AVR programs often end with a jump-to-self after calling main()
    // so we make that particular opcode return a special result
    // from cpu_run() instead of looping forever
    Instr[0xcfff] = do_halt;

    cpu_reset();
}

void cpu_load_flash(u8 *buf, u32 bufsize)
{
    memcpy(Program, buf, bufsize);
}

void cpu_load_eeprom(u8 *buf, u32 bufsize)
{
    eeprom_load(buf, bufsize);
}

void cpu_usart_set_output(int fd)
{
    usart_set_output(fd);
}

void cpu_usart_set_input(int fd)
{
    usart_set_input(fd);
}

void cpu_reset()
{
    PC = 0;
    Cycle = 0;
    Data.SP = DATA_SIZE_BYTES - 1;
    LastPoll = 0;
    State = CPU_RUN;
}

int cpu_run()
{
    while (State == CPU_RUN) {
        #ifdef TRACE
            int i;
            for (i = 0; i < 24; i++) {
                fprintf(stderr, "%2d:%02x ", i, Data.Reg[i]);
                if (i == 15) {
                    fprintf(stderr, "\n");
                }
            }
            for (i = 0; i < 4; i++) {
                fprintf(stderr, "%d:%04x ", 24+i*2, Data.RegW[i]);
            }
            fprintf(stderr, "SP:%04x ", Data.SP);
            for (i = 7; i >= 0; i--) {
                static const char flags[] = "cznvshti";
                putc(Data.SREG.bits & (1 << i) ? toupper(flags[i]) : flags[i], stderr);
            }
            fprintf(stderr, "\n");
            fprintf(stderr, "%04x %04x ", PC*2, Program[PC]);
        #endif
        u16 instr = Program[PC++];
        Instr[instr](instr);
        if (Cycle - LastPoll > 10000) {
            LastPoll = Cycle;
            int i;
            for (i = 0; i < PollFunctionCount; i++) {
                PollFunctions[i]();
            }
            break;
        }
    }
    return State;
}

void cpu_set_pin(int pin, bool state)
{
    assert(pin >= 0);
    assert(pin < LENGTHOF(PinCallback));
    port_pin(pin, state);
}

void cpu_pin_callback(int pin, PinFunction f)
{
    assert(pin >= 0);
    assert(pin < LENGTHOF(PinCallback));
    PinCallback[pin] = f;
}

u32 cpu_get_cycles()
{
    return Cycle;
}
