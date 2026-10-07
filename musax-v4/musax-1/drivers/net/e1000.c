#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/arch/x86_64/paging.h>
#include <musax/drivers/e1000.h>
#include <musax/drivers/pci.h>
#include <musax/drivers/timer.h>
#include <musax/pmm.h>

#define INTEL_VENDOR 0x8086
#define DEVICE_82540EM 0x100E
#define DEVICE_82545EM 0x100F

#define REG_CTRL 0x0000
#define REG_STATUS 0x0008
#define REG_EERD 0x0014
#define REG_IMC 0x00D8
#define REG_RCTL 0x0100
#define REG_TCTL 0x0400
#define REG_TIPG 0x0410
#define REG_RDBAL 0x2800
#define REG_RDBAH 0x2804
#define REG_RDLEN 0x2808
#define REG_RDH 0x2810
#define REG_RDT 0x2818
#define REG_TDBAL 0x3800
#define REG_TDBAH 0x3804
#define REG_TDLEN 0x3808
#define REG_TDH 0x3810
#define REG_TDT 0x3818
#define REG_MTA 0x5200
#define REG_RAL0 0x5400
#define REG_RAH0 0x5404

#define MTA_ENTRIES 128
#define MMIO_BYTES 0x20000

#define CTRL_ASDE (1U << 5)
#define CTRL_SLU (1U << 6)
#define CTRL_RESET (1U << 26)
#define STATUS_LINK_UP (1U << 1)
#define EERD_START 0x1
#define EERD_DONE (1U << 4)
#define RCTL_ENABLE (1U << 1)
#define RCTL_BROADCAST_ACCEPT (1U << 15)
#define RCTL_STRIP_CRC (1U << 26)
#define TCTL_ENABLE (1U << 1)
#define TCTL_PAD_SHORT (1U << 3)
#define TCTL_COLLISION_THRESHOLD (0x10U << 4)
#define TCTL_COLLISION_DISTANCE (0x40U << 12)
#define TIPG_DEFAULT 0x0060200AU
#define RAH_ADDRESS_VALID (1U << 31)

#define RX_STATUS_DONE 0x01
#define TX_CMD_END_OF_PACKET 0x01
#define TX_CMD_INSERT_FCS 0x02
#define TX_CMD_REPORT_STATUS 0x08
#define TX_STATUS_DONE 0x01

#define RX_RING 32
#define TX_RING 16
#define BUFFER_SIZE 2048
#define FRAME_SIZE 4096
#define RESET_SETTLE_MS 20
#define TX_WAIT_MS 20
#define EEPROM_SPINS 100000

struct rx_descriptor {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct tx_descriptor {
    uint64_t address;
    uint16_t length;
    uint8_t checksum_offset;
    uint8_t command;
    uint8_t status;
    uint8_t checksum_start;
    uint16_t special;
} __attribute__((packed));

static volatile uint32_t *registers;
static struct rx_descriptor *rx_ring;
static struct tx_descriptor *tx_ring;
static uint8_t *rx_buffers;
static uint8_t *tx_buffers;
static unsigned rx_index;
static unsigned tx_index;

static uint32_t read_register(uint32_t offset)
{
    return registers[offset / 4];
}

static void write_register(uint32_t offset, uint32_t value)
{
    registers[offset / 4] = value;
}

static bool read_eeprom_word(unsigned address, uint16_t *word)
{
    write_register(REG_EERD, EERD_START | ((uint32_t)address << 8));
    for (int spins = 0; spins < EEPROM_SPINS; spins++) {
        uint32_t value = read_register(REG_EERD);

        if (value & EERD_DONE) {
            *word = (uint16_t)(value >> 16);
            return true;
        }
    }
    return false;
}

static void read_mac(uint8_t mac[6])
{
    uint32_t low = read_register(REG_RAL0);
    uint32_t high = read_register(REG_RAH0);

    if (low == 0 && (high & 0xFFFF) == 0) {
        for (unsigned i = 0; i < 3; i++) {
            uint16_t word = 0;

            read_eeprom_word(i, &word);
            mac[2 * i] = (uint8_t)word;
            mac[2 * i + 1] = (uint8_t)(word >> 8);
        }
        return;
    }
    mac[0] = (uint8_t)low;
    mac[1] = (uint8_t)(low >> 8);
    mac[2] = (uint8_t)(low >> 16);
    mac[3] = (uint8_t)(low >> 24);
    mac[4] = (uint8_t)high;
    mac[5] = (uint8_t)(high >> 8);
}

static int e1000_send(const void *frame, size_t length)
{
    struct tx_descriptor *descriptor = &tx_ring[tx_index];

    if (length > BUFFER_SIZE)
        return -1;
    for (int waited = 0; !(descriptor->status & TX_STATUS_DONE); waited++) {
        if (waited >= TX_WAIT_MS * 1000)
            return -1;
    }

    memcpy(tx_buffers + (size_t)tx_index * BUFFER_SIZE, frame, length);
    descriptor->length = (uint16_t)length;
    descriptor->command = TX_CMD_END_OF_PACKET | TX_CMD_INSERT_FCS | TX_CMD_REPORT_STATUS;
    descriptor->status = 0;

    tx_index = (tx_index + 1) % TX_RING;
    write_register(REG_TDT, tx_index);
    return 0;
}

static int e1000_receive(void *buffer, size_t capacity)
{
    struct rx_descriptor *descriptor = &rx_ring[rx_index];
    size_t length;
    unsigned finished;

    if (!(descriptor->status & RX_STATUS_DONE))
        return 0;

    length = descriptor->length;
    if (length > capacity)
        length = capacity;
    memcpy(buffer, rx_buffers + (size_t)rx_index * BUFFER_SIZE, length);

    descriptor->status = 0;
    finished = rx_index;
    rx_index = (rx_index + 1) % RX_RING;
    write_register(REG_RDT, finished);
    return (int)length;
}

static bool e1000_link_up(void)
{
    return (read_register(REG_STATUS) & STATUS_LINK_UP) != 0;
}

bool e1000_init(struct netdev *device)
{
    const struct pci_device *adapter = pci_find(INTEL_VENDOR, DEVICE_82540EM);
    uint64_t bar;
    uint64_t memory;
    uint64_t rx_physical;
    uint64_t tx_physical;
    uint8_t *base;
    size_t rx_frames = (size_t)RX_RING * BUFFER_SIZE / FRAME_SIZE;
    size_t tx_frames = (size_t)TX_RING * BUFFER_SIZE / FRAME_SIZE;

    if (adapter == NULL)
        adapter = pci_find(INTEL_VENDOR, DEVICE_82545EM);
    if (adapter == NULL)
        return false;

    bar = pci_memory_bar(adapter, 0);
    if (bar == 0 || bar >= 0x100000000ULL)
        return false;
    pci_enable_device(adapter);
    paging_set_uncached(bar, MMIO_BYTES);
    registers = (volatile uint32_t *)(uintptr_t)bar;

    memory = pmm_alloc_contiguous(1 + rx_frames + tx_frames);
    if (memory == 0)
        return false;
    base = (uint8_t *)(uintptr_t)memory;
    memset(base, 0, FRAME_SIZE);
    rx_ring = (struct rx_descriptor *)base;
    tx_ring = (struct tx_descriptor *)(base + RX_RING * sizeof(struct rx_descriptor));
    rx_buffers = base + FRAME_SIZE;
    tx_buffers = rx_buffers + rx_frames * FRAME_SIZE;
    rx_physical = memory + (uint64_t)0;
    tx_physical = memory + RX_RING * sizeof(struct rx_descriptor);

    write_register(REG_IMC, 0xFFFFFFFFU);
    write_register(REG_CTRL, read_register(REG_CTRL) | CTRL_RESET);
    timer_delay_ms(RESET_SETTLE_MS);
    write_register(REG_IMC, 0xFFFFFFFFU);
    write_register(REG_CTRL, (read_register(REG_CTRL) | CTRL_SLU | CTRL_ASDE) & ~CTRL_RESET);

    read_mac(device->mac);
    write_register(REG_RAL0, (uint32_t)device->mac[0] | ((uint32_t)device->mac[1] << 8) |
                                 ((uint32_t)device->mac[2] << 16) | ((uint32_t)device->mac[3] << 24));
    write_register(REG_RAH0, (uint32_t)device->mac[4] | ((uint32_t)device->mac[5] << 8) | RAH_ADDRESS_VALID);
    for (unsigned i = 0; i < MTA_ENTRIES; i++)
        write_register(REG_MTA + 4 * i, 0);

    for (unsigned i = 0; i < RX_RING; i++)
        rx_ring[i].address = (uint64_t)(uintptr_t)(rx_buffers + (size_t)i * BUFFER_SIZE);
    write_register(REG_RDBAL, (uint32_t)rx_physical);
    write_register(REG_RDBAH, (uint32_t)(rx_physical >> 32));
    write_register(REG_RDLEN, RX_RING * sizeof(struct rx_descriptor));
    write_register(REG_RDH, 0);
    write_register(REG_RDT, RX_RING - 1);
    rx_index = 0;
    write_register(REG_RCTL, RCTL_ENABLE | RCTL_BROADCAST_ACCEPT | RCTL_STRIP_CRC);

    for (unsigned i = 0; i < TX_RING; i++) {
        tx_ring[i].address = (uint64_t)(uintptr_t)(tx_buffers + (size_t)i * BUFFER_SIZE);
        tx_ring[i].status = TX_STATUS_DONE;
    }
    write_register(REG_TDBAL, (uint32_t)tx_physical);
    write_register(REG_TDBAH, (uint32_t)(tx_physical >> 32));
    write_register(REG_TDLEN, TX_RING * sizeof(struct tx_descriptor));
    write_register(REG_TDH, 0);
    write_register(REG_TDT, 0);
    tx_index = 0;
    write_register(REG_TIPG, TIPG_DEFAULT);
    write_register(REG_TCTL, TCTL_ENABLE | TCTL_PAD_SHORT | TCTL_COLLISION_THRESHOLD | TCTL_COLLISION_DISTANCE);

    strcpy(device->name, "eth0");
    device->send = e1000_send;
    device->receive = e1000_receive;
    device->link_up = e1000_link_up;
    return true;
}
