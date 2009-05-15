#include "timer.h"

#include "cpu.h"

#define TIMER_IRQ   17

void timer_poll()
{
    irq(TIMER_IRQ);
}

void timer_init()
{
    register_poll(timer_poll);
}
