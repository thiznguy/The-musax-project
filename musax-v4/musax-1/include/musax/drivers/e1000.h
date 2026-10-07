#ifndef MUSAX_DRIVERS_E1000_H
#define MUSAX_DRIVERS_E1000_H

#include <stdbool.h>
#include <musax/net/net.h>

/* Finds an Intel e1000 family adapter on the PCI bus and brings it up; fills in device. Returns false when none exists. */
bool e1000_init(struct netdev *device);

#endif
