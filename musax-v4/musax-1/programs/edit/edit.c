#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/fs.h>
#include <musax/keys.h>
#include <musax/programs/edit.h>
#include <musax/screen.h>
#include "buffer.h"

#define DEFAULT_TAB_WIDTH 4
#define MAX_TAB_WIDTH 16
#define DEFAULT_MESSAGE "Ctrl-S save  Ctrl-Q quit  arrows/Home/End/PgUp/PgDn move"

struct editor {
    struct text_buffer buffer;
    char path[FS_PATH_MAX];
    size_t cursor_row;
    size_t cursor_column;
    size_t row_offset;
    size_t column_offset;
    unsigned tab_width;
    bool modified;
    bool quit_requested;
    char message[CONSOLE_COLUMNS + 1];
};

static void set_message(struct editor *editor, const char *text)
{
    strlcpy(editor->message, text, sizeof(editor->message));
}

static size_t text_rows(void)
{
    return console_rows() - 2;
}

static size_t current_line_length(const struct editor *editor)
{
    return editor->buffer.lines[editor->cursor_row].length;
}

static void clamp_cursor(struct editor *editor)
{
    if (editor->cursor_row >= editor->buffer.line_count)
        editor->cursor_row = editor->buffer.line_count - 1;
    if (editor->cursor_column > current_line_length(editor))
        editor->cursor_column = current_line_length(editor);
}

static void scroll_to_cursor(struct editor *editor)
{
    if (editor->cursor_row < editor->row_offset)
        editor->row_offset = editor->cursor_row;
    if (editor->cursor_row >= editor->row_offset + text_rows())
        editor->row_offset = editor->cursor_row - text_rows() + 1;
    if (editor->cursor_column < editor->column_offset)
        editor->column_offset = editor->cursor_column;
    if (editor->cursor_column >= editor->column_offset + CONSOLE_COLUMNS)
        editor->column_offset = editor->cursor_column - CONSOLE_COLUMNS + 1;
}

static char display_char(char c)
{
    if (c == '\t')
        return ' ';
    if (c >= 0x20 && c < 0x7F)
        return c;
    return '?';
}

static void render(struct editor *editor)
{
    char row_text[CONSOLE_COLUMNS];
    char status[CONSOLE_COLUMNS + 1];

    scroll_to_cursor(editor);
    screen_clear(CONSOLE_ATTRIBUTE_NORMAL);

    for (size_t row = 0; row < text_rows(); row++) {
        size_t file_row = editor->row_offset + row;
        const struct text_line *line;
        size_t visible = 0;

        if (file_row >= editor->buffer.line_count) {
            screen_write(row, 0, "~", 1, CONSOLE_ATTRIBUTE_NORMAL);
            continue;
        }

        line = &editor->buffer.lines[file_row];
        for (size_t column = editor->column_offset; column < line->length && visible < CONSOLE_COLUMNS; column++)
            row_text[visible++] = display_char(line->text[column]);
        screen_write(row, 0, row_text, visible, CONSOLE_ATTRIBUTE_NORMAL);
    }

    snprintf(status, sizeof(status), " edit  %s%s  Ln %zu/%zu  Col %zu", editor->path,
             editor->modified ? " [modified]" : "", editor->cursor_row + 1, editor->buffer.line_count,
             editor->cursor_column + 1);
    screen_fill_row(text_rows(), ' ', CONSOLE_ATTRIBUTE_INVERSE);
    screen_write(text_rows(), 0, status, strlen(status), CONSOLE_ATTRIBUTE_INVERSE);
    screen_write(text_rows() + 1, 0, editor->message, strlen(editor->message), CONSOLE_ATTRIBUTE_NORMAL);

    screen_flush(editor->cursor_row - editor->row_offset, editor->cursor_column - editor->column_offset);
}

static void save(struct editor *editor)
{
    size_t size;
    uint8_t *data = text_buffer_serialize(&editor->buffer, &size);
    int error;

    if (data == NULL) {
        set_message(editor, "Save failed: out of memory");
        return;
    }
    error = fs_write_file(editor->path, data, size);
    free(data);

    if (error != FS_OK) {
        snprintf(editor->message, sizeof(editor->message), "Save failed: %s", fs_error_message(error));
        return;
    }
    editor->modified = false;
    snprintf(editor->message, sizeof(editor->message), "Saved %zu bytes", size);
}

static void insert_char(struct editor *editor, char c)
{
    if (text_buffer_insert_char(&editor->buffer, editor->cursor_row, editor->cursor_column, c) != 0) {
        set_message(editor, "Out of memory");
        return;
    }
    editor->cursor_column++;
    editor->modified = true;
}

static void insert_newline(struct editor *editor)
{
    if (text_buffer_split_line(&editor->buffer, editor->cursor_row, editor->cursor_column) != 0) {
        set_message(editor, "Out of memory");
        return;
    }
    editor->cursor_row++;
    editor->cursor_column = 0;
    editor->modified = true;
}

static void move_left(struct editor *editor)
{
    if (editor->cursor_column > 0) {
        editor->cursor_column--;
    } else if (editor->cursor_row > 0) {
        editor->cursor_row--;
        editor->cursor_column = current_line_length(editor);
    }
}

static void move_right(struct editor *editor)
{
    if (editor->cursor_column < current_line_length(editor)) {
        editor->cursor_column++;
    } else if (editor->cursor_row + 1 < editor->buffer.line_count) {
        editor->cursor_row++;
        editor->cursor_column = 0;
    }
}

static bool handle_key(struct editor *editor, int key)
{
    bool was_quit_requested = editor->quit_requested;

    editor->quit_requested = false;

    switch (key) {
    case KEY_CTRL('Q'):
        if (editor->modified && !was_quit_requested) {
            editor->quit_requested = true;
            set_message(editor, "Unsaved changes: press Ctrl-Q again to quit without saving");
            return true;
        }
        return false;
    case KEY_CTRL('S'):
        save(editor);
        return true;
    case KEY_UP:
        if (editor->cursor_row > 0)
            editor->cursor_row--;
        break;
    case KEY_DOWN:
        if (editor->cursor_row + 1 < editor->buffer.line_count)
            editor->cursor_row++;
        break;
    case KEY_LEFT:
        move_left(editor);
        break;
    case KEY_RIGHT:
        move_right(editor);
        break;
    case KEY_HOME:
        editor->cursor_column = 0;
        break;
    case KEY_END:
        editor->cursor_column = current_line_length(editor);
        break;
    case KEY_PAGE_UP:
        editor->cursor_row = editor->cursor_row > text_rows() ? editor->cursor_row - text_rows() : 0;
        break;
    case KEY_PAGE_DOWN:
        editor->cursor_row += text_rows();
        break;
    case KEY_ENTER:
        insert_newline(editor);
        break;
    case KEY_BACKSPACE:
        if (editor->cursor_row > 0 || editor->cursor_column > 0) {
            if (text_buffer_delete_before(&editor->buffer, &editor->cursor_row, &editor->cursor_column) != 0)
                set_message(editor, "Out of memory");
            editor->modified = true;
        }
        break;
    case KEY_DELETE:
        if (text_buffer_delete_at(&editor->buffer, editor->cursor_row, editor->cursor_column) != 0)
            set_message(editor, "Out of memory");
        editor->modified = true;
        break;
    case KEY_TAB:
        for (unsigned i = 0; i < editor->tab_width; i++)
            insert_char(editor, ' ');
        break;
    default:
        if (key >= 0x20 && key < 0x7F)
            insert_char(editor, (char)key);
        break;
    }

    clamp_cursor(editor);
    return true;
}

static int load_file(struct editor *editor)
{
    const struct fs_node *node = fs_lookup(editor->path);
    uint8_t *data;
    int read;
    int result;

    if (node == NULL)
        return FS_OK;
    if (node->type != FS_NODE_FILE)
        return FS_ERR_IS_DIRECTORY;

    data = malloc(node->file.size > 0 ? node->file.size : 1);
    if (data == NULL)
        return FS_ERR_NO_MEMORY;
    read = fs_read(node, 0, data, node->file.size);
    result = read < 0 ? read : (text_buffer_load(&editor->buffer, data, (size_t)read) == 0 ? FS_OK : FS_ERR_NO_MEMORY);
    free(data);
    return result;
}

int edit_run(const char *path)
{
    if (console_is_graphical())
        return FS_ERR_INVALID;

    struct editor editor;
    long tab_width = config_get_long("editor.tab_width", DEFAULT_TAB_WIDTH);
    int error;

    memset(&editor, 0, sizeof(editor));
    if (strlen(path) >= sizeof(editor.path))
        return FS_ERR_NAME_TOO_LONG;
    strcpy(editor.path, path);
    editor.tab_width = tab_width >= 1 && tab_width <= MAX_TAB_WIDTH ? (unsigned)tab_width : DEFAULT_TAB_WIDTH;

    if (text_buffer_init(&editor.buffer) != 0)
        return FS_ERR_NO_MEMORY;
    error = load_file(&editor);
    if (error != FS_OK) {
        text_buffer_free(&editor.buffer);
        return error;
    }

    set_message(&editor, DEFAULT_MESSAGE);
    screen_begin();
    do {
        render(&editor);
    } while (handle_key(&editor, console_read_key()));
    screen_end();

    text_buffer_free(&editor.buffer);
    return FS_OK;
}
