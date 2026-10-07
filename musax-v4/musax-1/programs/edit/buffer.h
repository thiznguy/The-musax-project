#ifndef EDIT_BUFFER_H
#define EDIT_BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct text_line {
    char *text;
    size_t length;
    size_t capacity;
};

/* Text held as an array of lines without line terminators. */
struct text_buffer {
    struct text_line *lines;
    size_t line_count;
    size_t line_capacity;
};

/* All mutating calls return 0, or -1 when memory runs out. */
int text_buffer_init(struct text_buffer *buffer);
void text_buffer_free(struct text_buffer *buffer);
int text_buffer_load(struct text_buffer *buffer, const uint8_t *data, size_t size);
uint8_t *text_buffer_serialize(const struct text_buffer *buffer, size_t *size);

int text_buffer_insert_char(struct text_buffer *buffer, size_t row, size_t column, char c);
int text_buffer_split_line(struct text_buffer *buffer, size_t row, size_t column);

/* Backspace: removes the character before the position, joining lines at column 0, and moves the position back. */
int text_buffer_delete_before(struct text_buffer *buffer, size_t *row, size_t *column);

/* Delete: removes the character at the position, joining the next line at end of line. */
int text_buffer_delete_at(struct text_buffer *buffer, size_t row, size_t column);

#endif
