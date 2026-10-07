ARCH ?= x86_64
CROSS_COMPILE ?=
HOSTCC ?= cc

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUT := $(ROOT)/build/out
TALL_OUT := $(ROOT)/build/out-tall
OBJ := $(OUT)/obj
TEST_OUT := $(OUT)/tests

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
OBJCOPY := $(CROSS_COMPILE)objcopy
LD := $(CROSS_COMPILE)ld
GCC_INCLUDE := $(shell $(CC) -print-file-name=include)

IMAGE := $(OUT)/musax-1.img
KERNEL_ELF := $(OUT)/kernel.elf
KERNEL_BIN := $(OUT)/kernel.bin
STAGE1_BIN := $(OUT)/stage1.bin
STAGE2_BIN := $(OUT)/stage2.bin

KERNEL_LINKER_SCRIPT := $(ROOT)/arch/$(ARCH)/kernel.ld

comma := ,
rel = $(patsubst $(ROOT)/%,%,$(wildcard $(1)))

KERNEL_C_SOURCES := $(call rel,\
	$(ROOT)/arch/$(ARCH)/*.c \
	$(ROOT)/kernel/core/*.c \
	$(ROOT)/kernel/mm/*.c \
	$(ROOT)/drivers/*/*.c \
	$(ROOT)/fs/*.c \
	$(ROOT)/gui/*.c \
	$(ROOT)/net/*.c \
	$(ROOT)/programs/*/*.c \
	$(ROOT)/shell/music/*.c \
	$(ROOT)/shell/music/commands/*.c)
KERNEL_S_SOURCES := $(call rel,$(ROOT)/arch/$(ARCH)/*.S $(ROOT)/config/*.S)
LIBC_SOURCES := $(call rel,$(ROOT)/lib/libc/*/*.c)
LIBPATH_SOURCES := $(call rel,$(ROOT)/lib/libpath/*.c)
LIBCONF_SOURCES := $(call rel,$(ROOT)/lib/libconf/*.c)

KERNEL_OBJECTS := $(KERNEL_C_SOURCES:%=$(OBJ)/%.o) $(KERNEL_S_SOURCES:%=$(OBJ)/%.o)
LIBC_ARCHIVE := $(OUT)/libc.a
LIBPATH_ARCHIVE := $(OUT)/libpath.a
LIBCONF_ARCHIVE := $(OUT)/libconf.a
LIBRARIES := $(LIBC_ARCHIVE) $(LIBPATH_ARCHIVE) $(LIBCONF_ARCHIVE)

ARCH_CFLAGS_x86_64 := -m64 -mcmodel=small -mno-red-zone -mgeneral-regs-only
INCLUDES := -nostdinc -isystem $(GCC_INCLUDE) -I$(ROOT)/include -I$(ROOT)/include/libc
CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns \
	$(ARCH_CFLAGS_$(ARCH)) $(INCLUDES) -MMD -MP
ASFLAGS := $(ARCH_CFLAGS_$(ARCH)) -I$(ROOT)/include -I$(ROOT)/config
LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none -Wl,-z,noexecstack -Wl,--no-warn-rwx-segments \
	-Wl,-T,$(KERNEL_LINKER_SCRIPT)

BOOT_ASFLAGS := -c -Wa,-I,$(ROOT)/boot/common
BOOT_LDFLAGS := -z noexecstack --no-warn-rwx-segments

KERNEL_SECTORS = $(shell echo $$((($$(stat -c %s $(KERNEL_BIN)) + 511) / 512)))
STAGE2_DEFSYM = -Wa,--defsym,KERNEL_SECTORS=$(KERNEL_SECTORS) $(if $(TALL),-Wa$(comma)--defsym$(comma)DISPLAY_TALL=1)

STAGE2_OBJECTS := $(OBJ)/boot/stage2/realmode.o $(OBJ)/boot/stage2/protected.o $(OBJ)/boot/stage2/longmode.o

.PHONY: all clean run test tall
all: $(IMAGE)

# Builds musax-1.img with a 80x50 text screen, which shows the crash picture at full size.
tall:
	$(MAKE) -C $(ROOT)/build TALL=1 OUT=$(TALL_OUT) all

$(OBJ)/%.c.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.S.o: $(ROOT)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJ)/config/default_config.S.o: $(ROOT)/config/musax.conf

$(LIBC_ARCHIVE): $(LIBC_SOURCES:%=$(OBJ)/%.o)
	@mkdir -p $(dir $@)
	rm -f $@
	$(AR) rcs $@ $^

$(LIBPATH_ARCHIVE): $(LIBPATH_SOURCES:%=$(OBJ)/%.o)
	@mkdir -p $(dir $@)
	rm -f $@
	$(AR) rcs $@ $^

$(LIBCONF_ARCHIVE): $(LIBCONF_SOURCES:%=$(OBJ)/%.o)
	@mkdir -p $(dir $@)
	rm -f $@
	$(AR) rcs $@ $^

$(KERNEL_ELF): $(KERNEL_OBJECTS) $(LIBRARIES) $(KERNEL_LINKER_SCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(KERNEL_OBJECTS) -Wl,--start-group $(LIBRARIES) -Wl,--end-group

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(OBJ)/boot/stage1/stage1.o: $(ROOT)/boot/stage1/stage1.S $(ROOT)/boot/common/layout.inc
	@mkdir -p $(dir $@)
	$(CC) $(BOOT_ASFLAGS) $< -o $@

$(STAGE1_BIN): $(OBJ)/boot/stage1/stage1.o $(ROOT)/boot/stage1/stage1.ld
	$(LD) $(BOOT_LDFLAGS) -T $(ROOT)/boot/stage1/stage1.ld -o $(OBJ)/boot/stage1/stage1.elf $<
	$(OBJCOPY) -O binary $(OBJ)/boot/stage1/stage1.elf $@
	test "$$(stat -c %s $@)" -eq 512

$(OBJ)/boot/stage2/%.o: $(ROOT)/boot/stage2/%.S $(ROOT)/boot/common/layout.inc $(KERNEL_BIN)
	@mkdir -p $(dir $@)
	$(CC) $(BOOT_ASFLAGS) $(STAGE2_DEFSYM) $< -o $@

$(STAGE2_BIN): $(STAGE2_OBJECTS) $(ROOT)/boot/stage2/stage2.ld
	$(LD) $(BOOT_LDFLAGS) -T $(ROOT)/boot/stage2/stage2.ld -o $(OBJ)/boot/stage2/stage2.elf $(STAGE2_OBJECTS)
	$(OBJCOPY) -O binary $(OBJ)/boot/stage2/stage2.elf $@

$(IMAGE): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	test $(KERNEL_SECTORS) -le 768
	dd if=/dev/zero of=$@ bs=512 count=$$((17 + $(KERNEL_SECTORS))) status=none
	dd if=$(STAGE1_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none

run: $(IMAGE)
	qemu-system-x86_64 -drive format=raw,file=$(IMAGE) -serial stdio -m 128M -vga std -nic user,model=e1000

TEST_CFLAGS := -std=gnu11 -O1 -g -Wall -Wextra -Werror -I$(ROOT)/include -I$(ROOT)/tests
LIBC_TEST_CFLAGS := -std=gnu11 -O1 -ffreestanding -fno-builtin -fno-stack-protector -nostdinc \
	-isystem $(GCC_INCLUDE) -I$(ROOT)/include -I$(ROOT)/include/libc
TESTS := libc path conf fs edit_buffer pmm paging tss crash_class net term_grid gfx wm

$(TEST_OUT)/libc_prefixed.o: $(LIBC_SOURCES:%=$(ROOT)/%)
	@mkdir -p $(TEST_OUT)/libc_objects
	for source in $^; do \
		$(HOSTCC) $(LIBC_TEST_CFLAGS) -c $$source -o $(TEST_OUT)/libc_objects/$$(basename $$source).o || exit 1; \
	done
	$(LD) -r -o $(TEST_OUT)/libc_combined.o $(TEST_OUT)/libc_objects/*.o
	$(OBJCOPY) --prefix-symbols=mx_ $(TEST_OUT)/libc_combined.o $@

$(TEST_OUT)/libc_test: $(ROOT)/tests/libc_test.c $(TEST_OUT)/libc_prefixed.o
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/path_test: $(ROOT)/tests/path_test.c $(ROOT)/lib/libpath/path.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/conf_test: $(ROOT)/tests/conf_test.c $(ROOT)/lib/libconf/conf.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/fs_test: $(ROOT)/tests/fs_test.c $(ROOT)/fs/fs.c $(ROOT)/fs/layout.c $(ROOT)/lib/libpath/path.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/edit_buffer_test: $(ROOT)/tests/edit_buffer_test.c $(ROOT)/programs/edit/buffer.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -I$(ROOT)/programs/edit -o $@ $^

$(TEST_OUT)/pmm_test: $(ROOT)/tests/pmm_test.c $(ROOT)/kernel/mm/pmm.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/paging_test: $(ROOT)/tests/paging_test.c $(ROOT)/arch/x86_64/paging.c $(ROOT)/kernel/mm/pmm.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/tss_test: $(ROOT)/tests/tss_test.c $(ROOT)/arch/x86_64/tss.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/crash_class_test: $(ROOT)/tests/crash_class_test.c $(ROOT)/kernel/core/crash_class.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/net_test: $(ROOT)/tests/net_test.c $(wildcard $(ROOT)/net/checksum.c $(ROOT)/net/ethernet.c $(ROOT)/net/arp.c $(ROOT)/net/ipv4.c $(ROOT)/net/icmp.c $(ROOT)/net/udp.c $(ROOT)/net/dhcp.c $(ROOT)/net/dns.c $(ROOT)/net/address.c)
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/term_grid_test: $(ROOT)/tests/term_grid_test.c $(ROOT)/gui/term_grid.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/gfx_test: $(ROOT)/tests/gfx_test.c $(ROOT)/gui/gfx.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

$(TEST_OUT)/wm_test: $(ROOT)/tests/wm_test.c $(ROOT)/gui/wm.c $(ROOT)/gui/gfx.c
	@mkdir -p $(TEST_OUT)
	$(HOSTCC) $(TEST_CFLAGS) -o $@ $^

test: $(TESTS:%=$(TEST_OUT)/%_test)
	@for test in $(TESTS); do $(TEST_OUT)/$${test}_test || exit 1; done

clean:
	rm -rf $(OUT) $(TALL_OUT)

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
