/* Boot-time physical memory layout and disk layout shared by stage 1 and stage 2. */

.set STAGE1_LOAD_ADDR, 0x7C00
.set STAGE2_LOAD_ADDR, 0x7E00
.set REAL_MODE_STACK_TOP, 0x7C00

/* Disk layout in 512-byte sectors: stage 1 at LBA 0, stage 2 at LBA 1, kernel image at LBA 17. */
.set STAGE2_LBA, 1
.set STAGE2_SECTORS, 16
.set KERNEL_LBA, 17
.set KERNEL_MAX_SECTORS, 768

/* Kernel staging buffer below 1 MiB, and the final kernel load address. */
.set KERNEL_BUFFER_SEGMENT, 0x2000
.set KERNEL_BUFFER_ADDR, 0x20000
.set KERNEL_LOAD_ADDR, 0x100000
.set KERNEL_READ_CHUNK_SECTORS, 64
.set KERNEL_READ_CHUNK_SEGMENT_STEP, 0x800

/* Boot information block handed to the kernel in RDI. */
.set BOOTINFO_ADDR, 0x1000
.set BOOTINFO_MAGIC, 0x5853554D
.set BOOTINFO_MAGIC_OFFSET, 0
.set BOOTINFO_COUNT_OFFSET, 4
.set BOOTINFO_DRIVE_OFFSET, 8
.set BOOTINFO_FONT_OFFSET, 12
.set BOOTINFO_ENTRIES_OFFSET, 16
.set BOOTINFO_ENTRY_SIZE, 24
.set BOOTINFO_MAX_ENTRIES, 64

/* Long mode page tables: PML4, PDPT and four page directories mapping the first 4 GiB with 2 MiB pages. */
.set PAGE_TABLE_PML4, 0x10000
.set PAGE_TABLE_PDPT, 0x11000
.set PAGE_TABLE_PD, 0x12000
.set PAGE_TABLE_BYTES, 0x6000

/* BIOS 8x16 text font copied below the boot information block: 256 glyphs of 16 bytes. */
.set FONT_ADDR, 0x2000
.set FONT_BYTES, 4096

/* Stage 2 GDT selectors. */
.set GDT_CODE32, 0x08
.set GDT_DATA32, 0x10
.set GDT_CODE64, 0x18
