/*
 * USART module for emulino
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

#include "usart.h"

#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

#include "cpu.h"

#define USART_UCSR0A    0xc0
#define USART_UCSR0B    0xc1
#define USART_UDR0      0xc6

#define USART_UCSRA_UDRE    BIT(5)
#define USART_UCSRA_TXC     BIT(6)
#define USART_UCSRA_RXC     BIT(7)

#define USART_UCSRB_RXCIE   BIT(7)

#define USART_IRQ   19

static int output = 0; // stdout
static int input = 1;  // stdin

u8 UCSRA;
u8 UCSRB;

u8 usart_read_ucsra(u16 addr)
{
    return UCSRA | USART_UCSRA_UDRE;
}

void usart_write_ucsra(u16 addr, u8 value)
{
    UCSRA &= ~(value & (USART_UCSRA_TXC));
}

u8 usart_read_ucsrb(u16 addr)
{
    return UCSRB;
}

void usart_write_ucsrb(u16 addr, u8 value)
{
    UCSRB = value;
    if (UCSRB & USART_UCSRB_RXCIE) {
    }
}

u8 usart_read_udr(u16 addr)
{
    if (UCSRA & USART_UCSRA_RXC) {
        UCSRA &= ~USART_UCSRA_RXC;
        u8 c;
        read(input, &c, 1);
        return c;
    } else {
        return 0;
    }
}

void usart_write_udr(u16 addr, u8 value)
{
    write(output, &value, 1);
}

void usart_poll()
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(input, &fds);
    struct timeval timeout = {0, 0};
    if (select(input+1, &fds, NULL, NULL, &timeout) > 0) {
        UCSRA |= USART_UCSRA_RXC;
        if (UCSRB & USART_UCSRB_RXCIE) {
            irq(USART_IRQ);
        }
    }
}

void usart_init()
{
    register_io(USART_UCSR0A, usart_read_ucsra, usart_write_ucsra);
    register_io(USART_UCSR0B, usart_read_ucsrb, usart_write_ucsrb);
    register_io(USART_UDR0, usart_read_udr, usart_write_udr);
    register_poll(usart_poll);
}

void usart_set_output(int f)
{
    output = f;
}

void usart_set_input(int f)
{
    input = f;
}
