#ifndef MUSAX_DRIVERS_TIMER_H
#define MUSAX_DRIVERS_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#define TIMER_TICK_HZ 1000

/* Programs the PIT as a free-running counter so timer_delay_ms works by polling. */
void timer_init(void);

/* Switches the PIT to an interrupt every millisecond; returns false when no tick arrived within a test window. */
bool timer_start_ticks(void);

bool timer_ticks_running(void);
uint64_t timer_uptime_ms(void);
void timer_delay_ms(uint32_t milliseconds);

/* Sleeps until the next tick when ticks run, otherwise returns right away. */
void timer_wait_for_tick(void);

/* Called by the interrupt handler on every tick. */
void timer_tick(void);

#endif
