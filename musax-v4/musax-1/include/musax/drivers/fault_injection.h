#ifndef MUSAX_DRIVERS_FAULT_INJECTION_H
#define MUSAX_DRIVERS_FAULT_INJECTION_H

/* Writes through a null pointer from inside driver code, to exercise driver crash detection. */
void fault_inject_driver_null_write(void);

#endif
