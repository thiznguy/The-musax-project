#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/drivers/pci.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_ENABLE_BIT 0x80000000U

#define PCI_REG_VENDOR 0x00
#define PCI_REG_COMMAND 0x04
#define PCI_REG_CLASS 0x08
#define PCI_REG_HEADER_TYPE 0x0E
#define PCI_REG_BAR0 0x10
#define PCI_REG_SECONDARY_BUS 0x19

#define PCI_HEADER_TYPE_MASK 0x7F
#define PCI_HEADER_MULTIFUNCTION 0x80
#define PCI_HEADER_BRIDGE 0x01
#define PCI_CLASS_BRIDGE 0x06
#define PCI_SUBCLASS_PCI_BRIDGE 0x04
#define PCI_NO_DEVICE 0xFFFF
#define PCI_BAR_IO 0x1
#define PCI_BAR_TYPE_MASK 0x6
#define PCI_BAR_TYPE_64BIT 0x4
#define PCI_BAR_MEMORY_MASK 0xFFFFFFF0U
#define PCI_SLOTS 32
#define PCI_FUNCTIONS 8
#define PCI_MAX_BUS_DEPTH 8

static struct pci_device devices[PCI_MAX_DEVICES];
static size_t device_count;

static uint32_t config_address(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
    return PCI_ENABLE_BIT | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)function << 8) |
           (offset & 0xFC);
}

static uint32_t raw_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
    outl(PCI_CONFIG_ADDRESS, config_address(bus, slot, function, offset));
    return inl(PCI_CONFIG_DATA);
}

static void raw_write32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t value)
{
    outl(PCI_CONFIG_ADDRESS, config_address(bus, slot, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

uint32_t pci_read32(const struct pci_device *device, uint8_t offset)
{
    return raw_read32(device->bus, device->slot, device->function, offset);
}

uint16_t pci_read16(const struct pci_device *device, uint8_t offset)
{
    return (uint16_t)(pci_read32(device, offset) >> ((offset & 2) * 8));
}

void pci_write32(const struct pci_device *device, uint8_t offset, uint32_t value)
{
    raw_write32(device->bus, device->slot, device->function, offset, value);
}

void pci_write16(const struct pci_device *device, uint8_t offset, uint16_t value)
{
    uint32_t shift = (offset & 2) * 8;
    uint32_t word = pci_read32(device, offset);

    word = (word & ~(0xFFFFU << shift)) | ((uint32_t)value << shift);
    pci_write32(device, offset, word);
}

static void scan_bus(uint8_t bus, unsigned depth);

static void scan_function(uint8_t bus, uint8_t slot, uint8_t function, unsigned depth)
{
    uint32_t id = raw_read32(bus, slot, function, PCI_REG_VENDOR);
    uint32_t class_word;
    uint8_t header_type;
    struct pci_device *device;

    if ((id & 0xFFFF) == PCI_NO_DEVICE || device_count == PCI_MAX_DEVICES)
        return;

    class_word = raw_read32(bus, slot, function, PCI_REG_CLASS);
    header_type = (uint8_t)(raw_read32(bus, slot, function, PCI_REG_HEADER_TYPE & 0xFC) >> 16) & PCI_HEADER_TYPE_MASK;

    device = &devices[device_count++];
    device->bus = bus;
    device->slot = slot;
    device->function = function;
    device->vendor_id = (uint16_t)(id & 0xFFFF);
    device->device_id = (uint16_t)(id >> 16);
    device->class_code = (uint8_t)(class_word >> 24);
    device->subclass = (uint8_t)(class_word >> 16);
    device->prog_if = (uint8_t)(class_word >> 8);
    for (unsigned i = 0; i < PCI_BAR_COUNT; i++)
        device->bar[i] = header_type == 0 ? raw_read32(bus, slot, function, (uint8_t)(PCI_REG_BAR0 + 4 * i)) : 0;

    if (header_type == PCI_HEADER_BRIDGE && device->class_code == PCI_CLASS_BRIDGE &&
        device->subclass == PCI_SUBCLASS_PCI_BRIDGE && depth < PCI_MAX_BUS_DEPTH) {
        uint8_t secondary = (uint8_t)(raw_read32(bus, slot, function, PCI_REG_SECONDARY_BUS & 0xFC) >> 8);

        if (secondary != 0 && secondary != bus)
            scan_bus(secondary, depth + 1);
    }
}

static void scan_bus(uint8_t bus, unsigned depth)
{
    for (uint8_t slot = 0; slot < PCI_SLOTS; slot++) {
        uint32_t id = raw_read32(bus, slot, 0, PCI_REG_VENDOR);
        uint8_t header_type;
        unsigned functions = 1;

        if ((id & 0xFFFF) == PCI_NO_DEVICE)
            continue;
        header_type = (uint8_t)(raw_read32(bus, slot, 0, PCI_REG_HEADER_TYPE & 0xFC) >> 16);
        if (header_type & PCI_HEADER_MULTIFUNCTION)
            functions = PCI_FUNCTIONS;
        for (uint8_t function = 0; function < functions; function++)
            scan_function(bus, slot, function, depth);
    }
}

void pci_init(void)
{
    device_count = 0;
    scan_bus(0, 0);
}

size_t pci_device_count(void)
{
    return device_count;
}

const struct pci_device *pci_device_at(size_t index)
{
    return index < device_count ? &devices[index] : NULL;
}

const struct pci_device *pci_find(uint16_t vendor_id, uint16_t device_id)
{
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id)
            return &devices[i];
    }
    return NULL;
}

void pci_enable_device(const struct pci_device *device)
{
    uint16_t command = pci_read16(device, PCI_REG_COMMAND);

    command |= PCI_COMMAND_IO_SPACE | PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER;
    pci_write16(device, PCI_REG_COMMAND, command);
}

uint64_t pci_memory_bar(const struct pci_device *device, unsigned index)
{
    uint32_t bar;
    uint64_t address;

    if (index >= PCI_BAR_COUNT)
        return 0;
    bar = device->bar[index];
    if (bar == 0 || (bar & PCI_BAR_IO))
        return 0;

    address = bar & PCI_BAR_MEMORY_MASK;
    if ((bar & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_64BIT && index + 1 < PCI_BAR_COUNT)
        address |= (uint64_t)device->bar[index + 1] << 32;
    return address;
}

const char *pci_class_name(uint8_t class_code, uint8_t subclass)
{
    switch (class_code) {
    case 0x01:
        return subclass == 0x01 ? "IDE controller" : subclass == 0x06 ? "SATA controller" : "storage controller";
    case 0x02:
        return "network controller";
    case 0x03:
        return "display controller";
    case 0x04:
        return "multimedia controller";
    case 0x06:
        return subclass == 0x00 ? "host bridge" : subclass == 0x01 ? "ISA bridge" : "bridge";
    case 0x0C:
        return subclass == 0x03 ? "USB controller" : "serial bus controller";
    default:
        return "device";
    }
}

const char *pci_device_name(uint16_t vendor_id, uint16_t device_id)
{
    if (vendor_id == 0x1234 && device_id == 0x1111)
        return "QEMU standard VGA";
    if (vendor_id == 0x8086 && device_id == 0x100E)
        return "Intel 82540EM (e1000)";
    if (vendor_id == 0x8086 && device_id == 0x1237)
        return "Intel 440FX host bridge";
    if (vendor_id == 0x8086 && device_id == 0x7000)
        return "Intel PIIX3 ISA bridge";
    if (vendor_id == 0x8086 && device_id == 0x7010)
        return "Intel PIIX3 IDE";
    if (vendor_id == 0x8086 && device_id == 0x7113)
        return "Intel PIIX4 ACPI";
    if (vendor_id == 0x8086 && device_id == 0x29C0)
        return "Intel Q35 host bridge";
    if (vendor_id == 0x10EC && device_id == 0x8139)
        return "Realtek RTL8139";
    if (vendor_id == 0x80EE && device_id == 0xBEEF)
        return "VirtualBox graphics adapter";
    return NULL;
}
