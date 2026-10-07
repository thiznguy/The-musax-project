#ifndef MUSAX_ARCH_CPUINFO_H
#define MUSAX_ARCH_CPUINFO_H

struct cpu_info {
    char vendor[13];
    char brand[49];
    unsigned family;
    unsigned model;
    unsigned stepping;
};

void arch_cpu_describe(struct cpu_info *info);

#endif
