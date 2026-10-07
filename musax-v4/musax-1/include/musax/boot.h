#ifndef MUSAX_BOOT_H
#define MUSAX_BOOT_H

#include <stdbool.h>
#include <stddef.h>

#define BOOT_MAX_STAGES 16
#define BOOT_DETAIL_MAX 28

/* One step of system startup. run returns 0 on success and describes its result in detail. */
struct boot_stage {
    const char *label;
    int (*run)(char *detail, size_t detail_size);
};

/* Runs every stage in order. With animated set, a splash screen shows the logo and each stage as it runs, and any key
   skips the animation; otherwise each stage is logged as a line of text. A failing stage ends in a kernel panic. */
void boot_run(const struct boot_stage *stages, size_t count, bool animated, unsigned frame_delay_ms);

/* Replays only the splash visuals, without stages. */
void boot_splash_demo(unsigned frame_delay_ms);

#endif
