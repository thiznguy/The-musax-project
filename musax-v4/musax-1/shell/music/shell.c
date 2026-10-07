#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <libpath/path.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/fs.h>
#include <musax/keys.h>
#include <musax/kprintf.h>
#include <musax/shell/music.h>
#include "internal.h"

#define LINE_MAX_LENGTH 128
#define HISTORY_SIZE 16
#define DEFAULT_PROMPT "muSIC %d>"
#define DEFAULT_HOSTNAME "musax"

const struct music_command music_commands[] = {
    { "help", "help [command]", "list commands or show one command's usage", command_help },
    { "version", "version", "print the musax-1 version", command_version },
    { "echo", "echo [text...]", "print the arguments", command_echo },
    { "clear", "clear", "clear the screen", command_clear },
    { "ls", "ls [-l] [path]", "list a directory", command_ls },
    { "cd", "cd [path]", "change the working directory", command_cd },
    { "pwd", "pwd", "print the working directory", command_pwd },
    { "mkdir", "mkdir [-p] path...", "create directories", command_mkdir },
    { "touch", "touch path...", "create empty files", command_touch },
    { "cat", "cat path...", "print files", command_cat },
    { "rm", "rm [-r] path...", "remove files and directories", command_rm },
    { "cp", "cp source destination", "copy a file", command_cp },
    { "mv", "mv source destination", "move or rename a file or directory", command_mv },
    { "tree", "tree [path]", "print a directory tree", command_tree },
    { "edit", "edit path", "open the full-screen text editor", command_edit },
    { "config", "config [get key | set key value | save | reload]", "show or change the configuration", command_config },
    { "hostname", "hostname", "print the host name", command_hostname },
    { "date", "date", "print the real-time clock", command_date },
    { "uptime", "uptime", "print the time since boot", command_uptime },
    { "sleep", "sleep seconds", "wait for a number of seconds", command_sleep },
    { "memmap", "memmap", "print the firmware memory map", command_memmap },
    { "free", "free", "print kernel heap usage", command_free },
    { "mem", "mem", "print physical memory frame usage", command_mem },
    { "cpuinfo", "cpuinfo", "print the processor model", command_cpuinfo },
    { "fsinfo", "fsinfo", "print filesystem usage", command_fsinfo },
    { "bootanim", "bootanim", "replay the boot animation", command_bootanim },
    { "desktop", "desktop", "start the graphical desktop", command_desktop },
    { "ifconfig", "ifconfig", "show the network adapter and its address", command_ifconfig },
    { "dhcp", "dhcp", "request an address from a DHCP server", command_dhcp },
    { "ping", "ping host [count]", "send echo requests to a host", command_ping },
    { "nslookup", "nslookup name", "look up the address of a host name", command_nslookup },
    { "arp", "arp", "show the ARP cache", command_arp },
    { "lspci", "lspci", "list PCI devices", command_lspci },
    { "crashtest", "crashtest driver|cpu|fault|panic", "trigger a crash to see the crash screen", command_crashtest },
    { "reboot", "reboot", "restart the machine", command_reboot },
    { "halt", "halt", "stop the processor", command_halt },
};

const size_t music_command_count = sizeof(music_commands) / sizeof(music_commands[0]);

char music_cwd[FS_PATH_MAX] = "/";

static char history[HISTORY_SIZE][LINE_MAX_LENGTH];
static size_t history_count;

/* Line being typed in the current session. */
static char line[LINE_MAX_LENGTH];
static size_t line_length;
static size_t history_index;

/* Growing byte buffer that collects command output for a redirection. */
struct capture {
    char *data;
    size_t length;
    size_t capacity;
    bool failed;
};

int music_resolve(const char *argument, char *out)
{
    return path_resolve(music_cwd, argument, out, FS_PATH_MAX);
}

void music_report(const char *command, const char *subject, int error)
{
    kprintf("%s: %s: %s\n", command, subject, fs_error_message(error));
}

static void print_prompt(void)
{
    const char *template = config_get("shell.prompt", DEFAULT_PROMPT);

    for (; *template != '\0'; template++) {
        if (*template != '%' || template[1] == '\0') {
            console_putc(*template);
            continue;
        }
        template++;
        switch (*template) {
        case 'd':
            console_puts(music_cwd);
            break;
        case 'h':
            console_puts(config_get("system.hostname", DEFAULT_HOSTNAME));
            break;
        case '%':
            console_putc('%');
            break;
        default:
            console_putc('%');
            console_putc(*template);
            break;
        }
    }
    console_putc(' ');
}

static void history_add(const char *line)
{
    if (line[0] == '\0')
        return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0)
        return;
    if (history_count == HISTORY_SIZE) {
        memmove(history[0], history[1], sizeof(history[0]) * (HISTORY_SIZE - 1));
        history_count--;
    }
    strcpy(history[history_count++], line);
}

static void replace_line(const char *replacement)
{
    while (line_length > 0) {
        console_putc('\b');
        line_length--;
    }
    while (replacement[line_length] != '\0') {
        line[line_length] = replacement[line_length];
        console_putc(line[line_length]);
        line_length++;
    }
    line[line_length] = '\0';
}

static int tokenize(char *line, char **argv, bool *quoted, int capacity)
{
    char *source = line;
    char *target = line;
    int argc = 0;

    for (;;) {
        bool more;

        while (*source == ' ' || *source == '\t')
            source++;
        if (*source == '\0')
            return argc;
        if (argc == capacity)
            return -1;

        argv[argc] = target;
        quoted[argc] = false;
        while (*source != '\0' && *source != ' ' && *source != '\t') {
            if (*source == '"') {
                quoted[argc] = true;
                source++;
                while (*source != '\0' && *source != '"')
                    *target++ = *source++;
                if (*source == '"')
                    source++;
            } else {
                *target++ = *source++;
            }
        }
        more = *source != '\0';
        if (more)
            source++;
        *target++ = '\0';
        argc++;
    }
}

static void capture_emit(char c, void *context)
{
    struct capture *capture = context;

    if (capture->failed)
        return;
    if (capture->length == capture->capacity) {
        size_t capacity = capture->capacity != 0 ? capture->capacity * 2 : 256;
        char *data = realloc(capture->data, capacity);

        if (data == NULL) {
            capture->failed = true;
            return;
        }
        capture->data = data;
        capture->capacity = capacity;
    }
    capture->data[capture->length++] = c;
}

static const struct music_command *find_command(const char *name)
{
    for (size_t i = 0; i < music_command_count; i++) {
        if (strcmp(name, music_commands[i].name) == 0)
            return &music_commands[i];
    }
    return NULL;
}

static void store_capture(const struct capture *capture, const char *target, bool append)
{
    char path[FS_PATH_MAX];
    int error;

    if (capture->failed) {
        kprintf("muSIC: output too large for memory\n");
        return;
    }
    if (music_resolve(target, path) != 0) {
        kprintf("muSIC: %s: path too long\n", target);
        return;
    }
    error = append ? fs_append_file(path, capture->data, capture->length)
                   : fs_write_file(path, capture->data, capture->length);
    if (error != FS_OK)
        music_report("muSIC", target, error);
}

static void execute(int argc, char **argv, const char *redirect_target, bool append)
{
    const struct music_command *command = find_command(argv[0]);
    struct capture capture = { NULL, 0, 0, false };

    if (command == NULL) {
        kprintf("muSIC: command not found: %s\n", argv[0]);
        return;
    }

    if (redirect_target == NULL) {
        command->handler(argc, argv);
        return;
    }

    console_push_sink(capture_emit, NULL, &capture);
    command->handler(argc, argv);
    console_pop_sink();
    store_capture(&capture, redirect_target, append);
    free(capture.data);
}

static int extract_redirection(int *argc, char **argv, const bool *quoted, const char **target, bool *append)
{
    for (int i = 0; i < *argc; i++) {
        bool is_append;

        if (quoted[i])
            continue;
        is_append = strcmp(argv[i], ">>") == 0;
        if (!is_append && strcmp(argv[i], ">") != 0)
            continue;
        if (i + 1 >= *argc || i == 0) {
            kprintf("muSIC: syntax error near '%s'\n", argv[i]);
            return -1;
        }
        *target = argv[i + 1];
        *append = is_append;
        *argc = i;
        return 0;
    }
    return 0;
}

static void run_line(void)
{
    char *argv[MUSIC_ARGUMENTS_MAX];
    bool quoted[MUSIC_ARGUMENTS_MAX];
    const char *redirect_target = NULL;
    bool append = false;
    int argc = tokenize(line, argv, quoted, MUSIC_ARGUMENTS_MAX);

    if (argc < 0) {
        kprintf("muSIC: too many arguments\n");
        return;
    }
    if (argc == 0)
        return;
    if (extract_redirection(&argc, argv, quoted, &redirect_target, &append) != 0)
        return;
    execute(argc, argv, redirect_target, append);
}

void music_session_start(void)
{
    line_length = 0;
    line[0] = '\0';
    history_index = history_count;
    print_prompt();
}

void music_session_key(int key)
{
    switch (key) {
    case KEY_ENTER:
        console_putc('\n');
        line[line_length] = '\0';
        history_add(line);
        run_line();
        music_session_start();
        break;
    case KEY_BACKSPACE:
        if (line_length > 0) {
            line_length--;
            console_putc('\b');
        }
        break;
    case KEY_UP:
        if (history_index > 0)
            replace_line(history[--history_index]);
        break;
    case KEY_DOWN:
        if (history_index < history_count) {
            history_index++;
            replace_line(history_index == history_count ? "" : history[history_index]);
        }
        break;
    case KEY_CTRL('C'):
        console_puts("^C\n");
        music_session_start();
        break;
    case KEY_CTRL('L'):
        console_clear();
        print_prompt();
        line[line_length] = '\0';
        console_puts(line);
        break;
    default:
        if (key >= 0x20 && key < 0x7F && line_length + 1 < sizeof(line)) {
            line[line_length++] = (char)key;
            console_putc((char)key);
        }
        break;
    }
}

void music_run(void)
{
    music_session_start();
    for (;;)
        music_session_key(console_read_key());
}
