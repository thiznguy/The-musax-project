#include <stdbool.h>
#include <stdint.h>
#include <musax/arch/cpu.h>
#include <musax/arch/x86_64/interrupts.h>
#include <musax/arch/x86_64/io.h>
#include <musax/arch/x86_64/pic.h>
#include <musax/drivers/timer.h>

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND 0x43
#define PIT_COMMAND_CHANNEL0_RATE_GENERATOR 0x34
#define PIT_COMMAND_LATCH_CHANNEL0 0x00
#define PIT_FREQUENCY_HZ 1193182ULL
#define PIT_FREE_RUNNING_PERIOD 65536U
#define PIT_IRQ 0
#define PIT_SELF_TEST_MS 50

static volatile uint64_t ticks;
static bool ticks_running;
static uint32_t counter_period = PIT_FREE_RUNNING_PERIOD;

static void program_reload(uint16_t reload)
{
    outb(PIT_COMMAND, PIT_COMMAND_CHANNEL0_RATE_GENERATOR);
    outb(PIT_CHANNEL0_DATA, (uint8_t)(reload & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)(reload >> 8));
    counter_period = reload != 0 ? reload : PIT_FREE_RUNNING_PERIOD;
}

static uint16_t read_counter(void)
{
    uint8_t low;
    uint8_t high;

    outb(PIT_COMMAND, PIT_COMMAND_LATCH_CHANNEL0);
    low = inb(PIT_CHANNEL0_DATA);
    high = inb(PIT_CHANNEL0_DATA);
    return (uint16_t)(((uint16_t)high << 8) | low);
}

static void delay_polling(uint32_t milliseconds)
{
    uint64_t target = (uint64_t)milliseconds * PIT_FREQUENCY_HZ / 1000ULL;
    uint64_t elapsed = 0;
    uint16_t previous = read_counter();

    while (elapsed < target) {
        uint16_t current = read_counter();

        elapsed += previous >= current ? (uint32_t)(previous - current) : previous + counter_period - current;
        previous = current;
        cpu_relax();
    }
}

void timer_init(void)
{
    program_reload(0);
}

void timer_tick(void)
{
    ticks++;
}

bool timer_start_ticks(void)
{
    uint64_t before;

    program_reload((uint16_t)(PIT_FREQUENCY_HZ / TIMER_TICK_HZ));
    before = ticks;
    irq_register(PIT_IRQ, timer_tick);
    pic_unmask(PIT_IRQ);
    cpu_enable_interrupts();

    delay_polling(PIT_SELF_TEST_MS);

    if (ticks == before) {
        cpu_disable_interrupts();
        pic_mask(PIT_IRQ);
        program_reload(0);
        return false;
    }
    ticks_running = true;
    return true;
}

bool timer_ticks_running(void)
{
    return ticks_running;
}

uint64_t timer_uptime_ms(void)
{
    return ticks;
}

void timer_delay_ms(uint32_t milliseconds)
{
    uint64_t deadline;

    if (!ticks_running || !cpu_interrupts_enabled()) {
        delay_polling(milliseconds);
        return;
    }
    deadline = ticks + milliseconds;
    while (ticks < deadline)
        cpu_idle();
}

void timer_wait_for_tick(void)
{
    uint64_t start = ticks;

    if (!ticks_running || !cpu_interrupts_enabled()) {
        cpu_relax();
        return;
    }
    while (ticks == start)
        cpu_idle();
}
