#ifndef MUSIC_INTERNAL_H
#define MUSIC_INTERNAL_H

#include <stddef.h>
#include <musax/fs.h>

#define MUSIC_ARGUMENTS_MAX 16

struct music_command {
    const char *name;
    const char *usage;
    const char *summary;
    int (*handler)(int argc, char **argv);
};

extern const struct music_command music_commands[];
extern const size_t music_command_count;

/* Current working directory as a normalized absolute path. */
extern char music_cwd[FS_PATH_MAX];

/* Resolves an argument against the working directory; returns 0, or -1 when the path is too long. */
int music_resolve(const char *argument, char *out);
void music_report(const char *command, const char *subject, int error);

int command_help(int argc, char **argv);
int command_version(int argc, char **argv);
int command_echo(int argc, char **argv);
int command_clear(int argc, char **argv);
int command_reboot(int argc, char **argv);
int command_halt(int argc, char **argv);
int command_memmap(int argc, char **argv);
int command_free(int argc, char **argv);
int command_date(int argc, char **argv);
int command_uptime(int argc, char **argv);
int command_sleep(int argc, char **argv);
int command_hostname(int argc, char **argv);
int command_bootanim(int argc, char **argv);
int command_mem(int argc, char **argv);
int command_cpuinfo(int argc, char **argv);
int command_crashtest(int argc, char **argv);
int command_desktop(int argc, char **argv);
int command_ifconfig(int argc, char **argv);
int command_dhcp(int argc, char **argv);
int command_ping(int argc, char **argv);
int command_nslookup(int argc, char **argv);
int command_arp(int argc, char **argv);
int command_lspci(int argc, char **argv);
int command_config(int argc, char **argv);
int command_ls(int argc, char **argv);
int command_cd(int argc, char **argv);
int command_pwd(int argc, char **argv);
int command_mkdir(int argc, char **argv);
int command_touch(int argc, char **argv);
int command_cat(int argc, char **argv);
int command_rm(int argc, char **argv);
int command_cp(int argc, char **argv);
int command_mv(int argc, char **argv);
int command_tree(int argc, char **argv);
int command_fsinfo(int argc, char **argv);
int command_edit(int argc, char **argv);

#endif
