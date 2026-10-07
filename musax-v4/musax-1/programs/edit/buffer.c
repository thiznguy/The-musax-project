#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

#define INITIAL_LINE_CAPACITY 16
#define INITIAL_LINE_COUNT 16

static int line_reserve(struct text_line *line, size_t needed)
{
    size_t capacity;
    char *text;

    if (line->capacity >= needed)
        return 0;
    capacity = line->capacity != 0 ? line->capacity : INITIAL_LINE_CAPACITY;
    while (capacity < needed)
        capacity *= 2;
    text = realloc(line->text, capacity);
    if (text == NULL)
        return -1;
    line->text = text;
    line->capacity = capacity;
    return 0;
}

static int line_append(struct text_line *line, const char *text, size_t length)
{
    if (line_reserve(line, line->length + length) != 0)
        return -1;
    if (length > 0)
        memcpy(line->text + line->length, text, length);
    line->length += length;
    return 0;
}

static int buffer_insert_line(struct text_buffer *buffer, size_t index)
{
    if (buffer->line_count == buffer->line_capacity) {
        size_t capacity = buffer->line_capacity * 2;
        struct text_line *lines = realloc(buffer->lines, capacity * sizeof(*lines));

        if (lines == NULL)
            return -1;
        buffer->lines = lines;
        buffer->line_capacity = capacity;
    }
    memmove(&buffer->lines[index + 1], &buffer->lines[index], (buffer->line_count - index) * sizeof(*buffer->lines));
    memset(&buffer->lines[index], 0, sizeof(buffer->lines[index]));
    buffer->line_count++;
    return 0;
}

static void buffer_remove_line(struct text_buffer *buffer, size_t index)
{
    free(buffer->lines[index].text);
    memmove(&buffer->lines[index], &buffer->lines[index + 1],
            (buffer->line_count - index - 1) * sizeof(*buffer->lines));
    buffer->line_count--;
}

int text_buffer_init(struct text_buffer *buffer)
{
    buffer->lines = calloc(INITIAL_LINE_COUNT, sizeof(*buffer->lines));
    if (buffer->lines == NULL)
        return -1;
    buffer->line_capacity = INITIAL_LINE_COUNT;
    buffer->line_count = 1;
    return 0;
}

void text_buffer_free(struct text_buffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; i++)
        free(buffer->lines[i].text);
    free(buffer->lines);
    buffer->lines = NULL;
    buffer->line_count = 0;
    buffer->line_capacity = 0;
}

int text_buffer_load(struct text_buffer *buffer, const uint8_t *data, size_t size)
{
    size_t row = 0;
    size_t start = 0;

    for (size_t i = 0; i <= size; i++) {
        if (i < size && data[i] != '\n')
            continue;
        if (line_append(&buffer->lines[row], (const char *)data + start, i - start) != 0)
            return -1;
        start = i + 1;
        if (i < size) {
            if (buffer_insert_line(buffer, row + 1) != 0)
                return -1;
            row++;
        }
    }
    return 0;
}

uint8_t *text_buffer_serialize(const struct text_buffer *buffer, size_t *size)
{
    size_t total = 0;
    size_t position = 0;
    uint8_t *data;

    for (size_t i = 0; i < buffer->line_count; i++)
        total += buffer->lines[i].length + (i > 0 ? 1 : 0);

    data = malloc(total > 0 ? total : 1);
    if (data == NULL)
        return NULL;

    for (size_t i = 0; i < buffer->line_count; i++) {
        if (i > 0)
            data[position++] = '\n';
        if (buffer->lines[i].length > 0)
            memcpy(data + position, buffer->lines[i].text, buffer->lines[i].length);
        position += buffer->lines[i].length;
    }
    *size = total;
    return data;
}

int text_buffer_insert_char(struct text_buffer *buffer, size_t row, size_t column, char c)
{
    struct text_line *line = &buffer->lines[row];

    if (line_reserve(line, line->length + 1) != 0)
        return -1;
    memmove(line->text + column + 1, line->text + column, line->length - column);
    line->text[column] = c;
    line->length++;
    return 0;
}

int text_buffer_split_line(struct text_buffer *buffer, size_t row, size_t column)
{
    struct text_line *line;
    struct text_line *tail;

    if (buffer_insert_line(buffer, row + 1) != 0)
        return -1;
    line = &buffer->lines[row];
    tail = &buffer->lines[row + 1];

    if (line_append(tail, line->text + column, line->length - column) != 0)
        return -1;
    line->length = column;
    return 0;
}

int text_buffer_delete_before(struct text_buffer *buffer, size_t *row, size_t *column)
{
    struct text_line *line = &buffer->lines[*row];

    if (*column > 0) {
        memmove(line->text + *column - 1, line->text + *column, line->length - *column);
        line->length--;
        (*column)--;
        return 0;
    }
    if (*row == 0)
        return 0;

    {
        struct text_line *previous = &buffer->lines[*row - 1];
        size_t join_column = previous->length;

        if (line_append(previous, line->text, line->length) != 0)
            return -1;
        buffer_remove_line(buffer, *row);
        (*row)--;
        *column = join_column;
    }
    return 0;
}

int text_buffer_delete_at(struct text_buffer *buffer, size_t row, size_t column)
{
    struct text_line *line = &buffer->lines[row];

    if (column < line->length) {
        memmove(line->text + column, line->text + column + 1, line->length - column - 1);
        line->length--;
        return 0;
    }
    if (row + 1 >= buffer->line_count)
        return 0;

    if (line_append(line, buffer->lines[row + 1].text, buffer->lines[row + 1].length) != 0)
        return -1;
    buffer_remove_line(buffer, row + 1);
    return 0;
}
