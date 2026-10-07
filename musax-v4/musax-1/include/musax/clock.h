#ifndef MUSAX_CLOCK_H
#define MUSAX_CLOCK_H

#include <stdbool.h>
#include <time.h>

/* Records the boot time from the real-time clock; returns false when the clock is unusable. */
bool clock_init(void);
bool clock_now(time_t *seconds_since_epoch);
bool clock_uptime(time_t *seconds);

#endif
