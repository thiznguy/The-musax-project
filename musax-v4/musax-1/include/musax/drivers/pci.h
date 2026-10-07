#ifndef MUSAX_DRIVERS_PCI_H
#define MUSAX_DRIVERS_PCI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PCI_MAX_DEVICES 64
#define PCI_BAR_COUNT 6

#define PCI_COMMAND_IO_SPACE 0x1
#define PCI_COMMAND_MEMORY_SPACE 0x2
#define PCI_COMMAND_BUS_MASTER 0x4

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar[PCI_BAR_COUNT];
};

/* Scans every bus reachable from bus 0 and records the devices found. */
void pci_init(void);

size_t pci_device_count(void);
const struct pci_device *pci_device_at(size_t index);
const struct pci_device *pci_find(uint16_t vendor_id, uint16_t device_id);

uint32_t pci_read32(const struct pci_device *device, uint8_t offset);
uint16_t pci_read16(const struct pci_device *device, uint8_t offset);
void pci_write32(const struct pci_device *device, uint8_t offset, uint32_t value);
void pci_write16(const struct pci_device *device, uint8_t offset, uint16_t value);

/* Turns on the memory space, I/O space and bus mastering bits of the command register. */
void pci_enable_device(const struct pci_device *device);

/* Base address of a memory BAR, or 0 when the BAR is unused or an I/O BAR. */
uint64_t pci_memory_bar(const struct pci_device *device, unsigned index);

const char *pci_class_name(uint8_t class_code, uint8_t subclass);
const char *pci_device_name(uint16_t vendor_id, uint16_t device_id);

#endif
