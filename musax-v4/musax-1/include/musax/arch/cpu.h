#ifndef MUSAX_ARCH_CPU_H
#define MUSAX_ARCH_CPU_H

#if defined(__x86_64__)
#include <musax/arch/x86_64/cpu.h>
#else
#error "musax-1 has no CPU support for this architecture"
#endif

#endif
