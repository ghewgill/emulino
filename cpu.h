typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned long u32;

typedef u8 (*ReadFunction)(u16 addr);
typedef void (*WriteFunction)(u16 addr, u8 value);

void register_io(u16 addr, ReadFunction rf, WriteFunction wf);
