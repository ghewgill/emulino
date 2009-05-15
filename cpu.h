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
