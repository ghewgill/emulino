#include <stdio.h>

#include "cpu.h"

u8 usart_read(u16 addr)
{
    return (1 << 5);
}

void usart_write(u16 addr, u8 value)
{
    putchar(value);
}

void usart_init()
{
    register_read(0xc0, usart_read);
    register_write(0xc6, usart_write);
}
