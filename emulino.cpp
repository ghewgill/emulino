#include <assert.h>
#include <ctype.h>
#include <stdio.h>

//#define TRACE

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;

typedef void (*Handler)(u16 instr);

const size_t PROGRAM_SIZE_WORDS = 0x1000;
const size_t DATA_SIZE_BYTES = 0x10000;

struct TData {
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
                    bool C: 1;
                    bool Z: 1;
                    bool N: 1;
                    bool V: 1;
                    bool S: 1;
                    bool H: 1;
                    bool T: 1;
                    bool I: 1;
                };
                u8 bits;
            } SREG;
        };
        u8 _Bytes[DATA_SIZE_BYTES];
        //u16 _Words[DATA_SIZE_BYTES/2];
    };
    inline u8 read(u16 addr) {
        if ((addr & 0xff00) == 0) {
            ioread(addr);
        }
        return _Bytes[addr];
    }
    inline void write(u16 addr, u8 value) {
        if ((addr & 0xff00) == 0) {
            iowrite(addr, value);
        } else {
            _Bytes[addr] = value;
        }
    }
    u8 ioread(u16 addr);
    void iowrite(u16 addr, u8 value);
};

u16 Program[PROGRAM_SIZE_WORDS];
TData Data;
u16 PC;

inline void trace(const char *s)
{
    #ifdef TRACE
        fprintf(stderr, "%s\n", s);
    #endif
}

void do_ADC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ADD(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ADIW(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_AND(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ANDI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ASR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BCLR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BLD(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BRBC(u16 instr)
{
    trace(__FUNCTION__);
    // ------kkkkkkksss
    u16 k = ((instr >> 3) & 0x7f);
    u16 s = (instr & 0x7);
    if ((Data.SREG.bits & (1 << s)) == 0) {
        PC += static_cast<s8>(k << 1) >> 1;
    }
}

void do_BRBS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BREAK(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BSET(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_BST(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_CALL(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_CBI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_COM(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_CP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_CPC(u16 instr)
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
    Data.SREG.Z &= x == 0;
    Data.SREG.C = (((~Data.Reg[d] & Data.Reg[r]) | (Data.Reg[r] & x) | (x & ~Data.Reg[d])) & 0x80) != 0;
}

void do_CPI(u16 instr)
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
}

void do_CPSE(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_DEC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_DES(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_EICALL(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_EIJMP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ELPM_1(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ELPM_2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ELPM_3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_EOR(u16 instr)
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
}

void do_FMUL(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_FMULS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_FMULSU(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ICALL(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_IJMP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_IN(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_INC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_JMP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_X1(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_X2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_X3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_Y2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_Y3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_Y4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-rrrrr-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 r = ((instr >> 4) & 0x1f);
    Data.Reg[r] = Data.read(Data.Y+q);
}

void do_LD_Z2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_Z3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LD_Z4(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LDI(u16 instr)
{
    trace(__FUNCTION__);
    // ----KKKKddddKKKK
    u16 K = (instr & 0xf) | ((instr >> 4) & 0xf0);
    u16 d = 16 + ((instr >> 4) & 0xf);
    Data.Reg[d] = K;
}

void do_LDS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LPM_1(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LPM_2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LPM_3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_LSR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_MOV(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_MOVW(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_MUL(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_MULS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_MULSU(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_NEG(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_NOP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_OR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ORI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_OUT(u16 instr)
{
    trace(__FUNCTION__);
    // -----AArrrrrAAAA
    u16 A = (instr & 0xf) | ((instr >> 5) & 0x30);
    u16 r = ((instr >> 4) & 0x1f);
    Data.write(0x20 + A, Data.Reg[r]);
}

void do_POP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_PUSH(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_RCALL(u16 instr)
{
    trace(__FUNCTION__);
    // ----kkkkkkkkkkkk
    u16 k = (instr & 0xfff);
    Data.write(Data.SP--, PC >> 8);
    Data.write(Data.SP--, PC & 0xff);
    PC += static_cast<s16>(k << 4) >> 4;
}

void do_RET(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_RETI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_RJMP(u16 instr)
{
    trace(__FUNCTION__);
    // ----kkkkkkkkkkkk
    u16 k = (instr & 0xfff);
    PC += static_cast<s16>(k << 4) >> 4;
}

void do_ROR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBCI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBI(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBIC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBIS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBIW(u16 instr)
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
}

void do_SBRC(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SBRS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SLEEP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SPM2_1(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SPM2_2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_X1(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_X2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_X3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_Y2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_Y3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_Y4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-rrrrr-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 r = ((instr >> 4) & 0x1f);
    Data.write(Data.Y+q, Data.Reg[r]);
}

void do_ST_Z2(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_Z3(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_ST_Z4(u16 instr)
{
    trace(__FUNCTION__);
    // --q-qq-rrrrr-qqq
    u16 q = (instr & 0x7) | ((instr >> 7) & 0x18) | ((instr >> 8) & 0x20);
    u16 r = ((instr >> 4) & 0x1f);
    Data.write(Data.Z+q, Data.Reg[r]);
}

void do_STS(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SUB(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_SUBI(u16 instr)
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
}

void do_SWAP(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

void do_WDR(u16)
{
    trace(__FUNCTION__);
    assert(false);
}

#include "avr.inc"

u8 TData::ioread(u16 addr)
{
    return _Bytes[addr];
}

void TData::iowrite(u16 addr, u8 value)
{
    fprintf(stderr, "iowrite %04x %02x\n", addr, value);
}

void LoadHex(const char *fn)
{
    FILE *f = fopen(fn, "r");
    char buf[100];
    bool eof = false;
    while (!eof && fgets(buf, sizeof(buf), f)) {
        assert(buf[0] == ':');
        u8 c = 0;
        for (int i = 1; isalnum(buf[i]); i += 2) {
            int x;
            sscanf(buf+i, "%02x", &x);
            c += x;
        }
        assert(c == 0);
        int n, a, t;
        sscanf(buf+1, "%02x%04x%02x", &n, &a, &t);
        switch (t) {
        case 0x00:
            for (int i = 0; i < n; i++) {
                int x;
                sscanf(buf+9+i*2, "%02x", &x);
                reinterpret_cast<u8 *>(Program)[a+i] = x;
            }
            break;
        case 0x01:
            eof = true;
            break;
        default:
            fprintf(stderr, "unsupported hex type: %02x\n", t);
            exit(1);
        }
    }
    fclose(f);
}

int main(int, char *[])
{
    assert(sizeof(Data.SREG) == 1);
    assert(reinterpret_cast<u8 *>(&Data.SP) - Data._Bytes == 0x5d);
    assert(reinterpret_cast<u8 *>(&Data.SREG) - Data._Bytes == 0x5f);

    //FILE *f = fopen("../blinky/blinky.bin", "rb");
    //fread(Program, 2, PROGRAM_SIZE_WORDS, f);
    //fclose(f);
    LoadHex("/Users/greg/arduino-0015/examples/Communication/ASCIITable/applet/ASCIITable.hex");

    PC = 0;
    Data.SP = DATA_SIZE_BYTES - 1;
    for (;;) {
        #ifdef TRACE
            fprintf(stderr, "%04x ", PC*2);
        #endif
        u16 instr = Program[PC];
        PC++;
        Instr[instr](instr);
    }
    return 0;
}
