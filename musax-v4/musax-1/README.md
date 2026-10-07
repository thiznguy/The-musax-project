#### THE MUSAX PROJECT ####

# musax-1

A portable kernel and operating system with its own shell, muSIC. The long-term goal is one kernel that runs as firmware for embedded projects and as a configurable server OS, with broad driver support on x86_64, ARM64 and RISC-V. Only x86_64 is implemented so far.

Current version: **0.0.0.4**

## What 0.0.0.4 adds

- **Desktop.** Type `desktop` at the muSIC prompt. The screen switches to 1024x768 with a gradient background and a top bar (Terminal and Settings buttons, a clock, and Exit).
  - **Alt+G** opens the terminal, a window running muSIC. Click the red **X** at the top right of a window to close it. Windows can be dragged by their title bar.
  - **Settings** (button or **Alt+S**) lets you click a color to change the background; the choice is stored as `desktop.background`.
  - **Alt+W** closes the focused window and **Alt+Q** (or the Exit button) returns to the text console.
  - It needs QEMU's standard VGA adapter and a PS/2 mouse (QEMU provides both; click inside the QEMU window to capture the mouse, Ctrl+Alt+G releases it).
  - `edit` and `bootanim` need the full-screen text console and say so when run inside the desktop terminal.
- **Networking.** PCI bus scan, an Intel e1000 driver, and a stack with ARP, IPv4, ICMP, UDP, a DHCP client and a DNS resolver. The address is requested automatically at boot (`net.dhcp = on`).
  - New commands: `ifconfig`, `dhcp`, `ping host [count]`, `nslookup name`, `arp`, `lspci`.
  - There is no TCP yet, so no web or SSH clients.
- **Input rework.** One PS/2 controller layer feeds both keyboard and mouse; Alt+key combinations exist.
- The boot loader now keeps the video BIOS font, which the desktop uses to draw text.
- Boot panel: "Starting timer and clock" and "Starting network" replace the separate clock step.

## What 0.0.0.3 added

- **Crash screen.** Any fatal fault shows a full-screen picture, the message "oppsies... your pc seems to have crashed" and an error code. Two causes are recognised for now:

  | Cause                     | Error code | How it is detected                                             |
  |---------------------------|------------|----------------------------------------------------------------|
  | Driver issue              | 09877KL    | the faulting instruction lies inside driver code (`drivers/`)  |
  | Faulty CPU / motherboard  | 56694MU    | machine check exception or non-maskable interrupt              |

  Any other crash shows the screen with "not identified yet" instead of a code. Try it with `crashtest driver`, `crashtest cpu`, `crashtest fault` or `crashtest panic`. Press any key on the crash screen to restart.
- **Boot screen.** Animated logo with shadow and shine, starfield, a startup panel where every line is a real initialization step with its result, a gradient progress bar and a wipe transition. Any key skips the animation. `boot.animation = off` replaces it with plain log lines.
- **Interrupts.** The interrupt controller is remapped and the system timer ticks at 1000 Hz. The processor sleeps between ticks instead of spinning. Keyboard and serial input are still polled, once per tick.
- **Crash-proof exception handling.** A task state segment gives double faults, NMIs and machine checks their own stacks, so a broken stack still produces a crash screen.
- **Physical memory manager.** A frame allocator built from the BIOS memory map; the heap is carved out of it.
- **Kernel-owned page tables.** Identity mapping of the first 4 GiB, with the first page unmapped so null-pointer access crashes instead of silently working.
- **New commands:** `mem`, `cpuinfo`, `crashtest`; `uptime` now uses the timer.
- **Colors.** The console supports 16 colors and VGA box/block characters (shown as UTF-8 on the serial port).

## Two screen sizes

`musax-1.img` uses the standard 80x25 text screen. The crash picture is 29 rows tall, so on this screen the kernel shows an 18-row version of the same drawing (repeated rows removed). `make -C build tall` builds `build/out-tall/musax-1.img`, which switches the BIOS to an 80x50 text screen at boot and shows the picture at full size. Characters are smaller on that screen.

## Earlier features

- Two-stage BIOS bootloader (`boot/stage1`, `boot/stage2`) that enters 64-bit mode.
- In-memory filesystem with musax-1's own directory names. Contents are lost on reboot.
- Configuration file at `/settings/musax.conf`.
- muSIC shell with history (Up/Down), quoting and `>` / `>>` redirection.
- Full-screen text editor (`edit`).
- A small C library (`lib/libc`) plus `libpath` and `libconf`.
- Drivers under `drivers/`: 16550 serial, VGA text, PS/2 keyboard, PIT timer, CMOS real-time clock, fault injection for crash tests.

## Directory names inside musax-1

| musax-1  | replaces | purpose                       |
|----------|----------|-------------------------------|
| /tools   | /bin     | programs                      |
| /system  | /sbin    | system programs               |
| /libs    | /lib     | libraries                     |
| /settings| /etc     | configuration                 |
| /devices | /dev     | devices                       |
| /users   | /home    | user data                     |
| /temp    | /tmp     | scratch space                 |
| /state   | /var     | data that changes while running |

The names live in `fs/layout.c`.

## muSIC commands

`help [command]`, `version`, `echo`, `clear`, `ls [-l] [path]`, `cd [path]`, `pwd`, `mkdir [-p]`, `touch`, `cat`, `rm [-r]`, `cp`, `mv`, `tree`, `edit path`, `config`, `hostname`, `date`, `uptime`, `sleep`, `memmap`, `free`, `mem`, `cpuinfo`, `fsinfo`, `bootanim`, `crashtest`, `desktop`, `ifconfig`, `dhcp`, `ping`, `nslookup`, `arp`, `lspci`, `reboot`, `halt`.

Redirection: `echo "hello world" > /temp/note.txt`. The `>` must be a separate word. Line editing: Backspace, Up/Down for history, Ctrl-C cancels the line, Ctrl-L clears the screen.

## Editor

`edit /temp/note.txt` opens a file, or starts a new one that is created on save. Arrows, Home, End, PgUp and PgDn move; Backspace and Delete delete; Tab inserts spaces; Ctrl-S saves; Ctrl-Q quits (press twice with unsaved changes).

## Configuration

`config` lists all settings, `config get key`, `config set key value`, `config save` writes `/settings/musax.conf`, `config reload` re-reads it. Defaults are in `config/musax.conf` and embedded in the kernel image.

| Key                      | Meaning                                      |
|--------------------------|----------------------------------------------|
| system.hostname          | host name (`%h` in the prompt)               |
| boot.animation           | `on` or `off`                                |
| boot.animation_delay_ms  | time per animation frame                     |
| shell.prompt             | prompt template; `%d` working directory, `%h` host name |
| shell.home               | directory used by `cd` with no argument      |
| editor.tab_width         | spaces inserted by Tab                       |

## Source layout

```
arch/x86_64/        entry point, GDT/TSS, IDT, interrupts, PIC, paging, CPUID, linker script
boot/               common constants, stage1 (MBR), stage2 (real, protected, long mode)
build/              Makefile (output goes to build/out/ and build/out-tall/)
config/             default configuration and the code that embeds it
drivers/            serial, video (text and Bochs display), input (keyboard, mouse), timer, rtc, bus (PCI), net (e1000), diagnostics
fs/                 in-memory filesystem and the standard directory layout
gui/                desktop, window manager, graphics, terminal and settings windows
net/                network protocols and the stack
include/            headers: libc, libpath, libconf, musax
kernel/core/        kmain, boot screen, crash screen, console, screen, config, clock, panic, kprintf
kernel/mm/          firmware memory map and the physical frame allocator
lib/                libc, libpath, libconf
programs/edit/      text editor
shell/music/        the muSIC shell and its commands
tests/              host-side unit tests
```

## Build, test, run

Requires GNU `gcc`, `ld`, `ar`, `objcopy`, `make` and `dd` on an x86_64 host, or a cross toolchain selected with `CROSS_COMPILE`.

```
make -C build          # builds build/out/musax-1.img
make -C build tall     # builds the 80x50 variant in build/out-tall/
make -C build test     # runs the unit tests on the host
make -C build run      # boots the image in QEMU with the standard VGA adapter and an e1000 network card
```

To see why a virtual machine resets or hangs, run QEMU with `-d int,cpu_reset -no-reboot`.

## Limits of this version

- The kernel image may be at most 768 sectors (384 KiB); the build fails beyond that.
- The filesystem is memory-only; there is no disk driver or on-disk format yet.
- Keyboard, mouse, serial and network input are polled, not interrupt-driven.
- The desktop drives the Bochs/QEMU display adapter only; other graphics hardware needs its own driver.
- The network stack has no TCP and the driver supports e1000 cards only.
- Memory above 4 GiB is not used.
- Only x86_64 with BIOS boot is implemented.


## Issues/problems of this version
- Desktop is unstable and cannot be relyed on
- Very limited hardware support
- Desktop red screens whenever you exit it