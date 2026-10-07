#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "check.h"

static char *dump(const struct text_buffer *buffer)
{
    static char text[256];
    size_t size;
    uint8_t *data = text_buffer_serialize(buffer, &size);

    memcpy(text, data, size);
    text[size] = '\0';
    free(data);
    return text;
}

static void load(struct text_buffer *buffer, const char *text)
{
    text_buffer_init(buffer);
    text_buffer_load(buffer, (const uint8_t *)text, strlen(text));
}

static void test_round_trip(void)
{
    static const char *const samples[] = { "", "a", "a\n", "\n", "\n\n", "one\ntwo\nthree", "one\ntwo\nthree\n", "x\n\ny" };

    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        struct text_buffer buffer;

        load(&buffer, samples[i]);
        CHECK_STR(dump(&buffer), samples[i]);
        text_buffer_free(&buffer);
    }
}

static void test_line_counts(void)
{
    struct text_buffer buffer;

    load(&buffer, "");
    CHECK(buffer.line_count == 1);
    text_buffer_free(&buffer);

    load(&buffer, "a\nb\n");
    CHECK(buffer.line_count == 3);
    CHECK(buffer.lines[2].length == 0);
    text_buffer_free(&buffer);
}

static void test_editing(void)
{
    struct text_buffer buffer;
    size_t row;
    size_t column;

    text_buffer_init(&buffer);
    for (const char *c = "hello"; *c != '\0'; c++)
        CHECK(text_buffer_insert_char(&buffer, 0, buffer.lines[0].length, *c) == 0);
    CHECK_STR(dump(&buffer), "hello");

    CHECK(text_buffer_insert_char(&buffer, 0, 0, '>') == 0);
    CHECK(text_buffer_insert_char(&buffer, 0, 3, '|') == 0);
    CHECK_STR(dump(&buffer), ">he|llo");

    CHECK(text_buffer_split_line(&buffer, 0, 3) == 0);
    CHECK_STR(dump(&buffer), ">he\n|llo");
    CHECK(buffer.line_count == 2);

    row = 1;
    column = 0;
    CHECK(text_buffer_delete_before(&buffer, &row, &column) == 0);
    CHECK(row == 0 && column == 3);
    CHECK_STR(dump(&buffer), ">he|llo");

    row = 0;
    column = 3;
    CHECK(text_buffer_delete_before(&buffer, &row, &column) == 0);
    CHECK(row == 0 && column == 2);
    CHECK_STR(dump(&buffer), ">h|llo");

    row = 0;
    column = 0;
    CHECK(text_buffer_delete_before(&buffer, &row, &column) == 0);
    CHECK(row == 0 && column == 0);
    CHECK_STR(dump(&buffer), ">h|llo");

    CHECK(text_buffer_delete_at(&buffer, 0, 0) == 0);
    CHECK_STR(dump(&buffer), "h|llo");
    CHECK(text_buffer_delete_at(&buffer, 0, 4) == 0);
    CHECK_STR(dump(&buffer), "h|ll");
    CHECK(text_buffer_delete_at(&buffer, 0, 4) == 0);
    CHECK_STR(dump(&buffer), "h|ll");

    CHECK(text_buffer_split_line(&buffer, 0, 2) == 0);
    CHECK_STR(dump(&buffer), "h|\nll");
    CHECK(buffer.line_count == 2);
    CHECK(text_buffer_delete_at(&buffer, 0, 2) == 0);
    CHECK_STR(dump(&buffer), "h|ll");
    CHECK(buffer.line_count == 1);

    text_buffer_free(&buffer);
}

static void test_many_lines_and_long_lines(void)
{
    struct text_buffer buffer;
    size_t size;
    uint8_t *data;

    text_buffer_init(&buffer);
    for (int i = 0; i < 500; i++) {
        CHECK(text_buffer_split_line(&buffer, buffer.line_count - 1, buffer.lines[buffer.line_count - 1].length) == 0);
        for (int j = 0; j < 40; j++)
            CHECK(text_buffer_insert_char(&buffer, buffer.line_count - 1, buffer.lines[buffer.line_count - 1].length,
                                          (char)('a' + j % 26)) == 0);
    }
    CHECK(buffer.line_count == 501);

    data = text_buffer_serialize(&buffer, &size);
    CHECK(size == 500 + 500 * 40);
    free(data);

    {
        size_t row;
        size_t column;

        while (buffer.line_count > 1) {
            row = buffer.line_count - 1;
            column = 0;
            CHECK(text_buffer_delete_before(&buffer, &row, &column) == 0);
        }
        CHECK(buffer.lines[0].length == 500 * 40);
    }
    text_buffer_free(&buffer);
}

int main(void)
{
    test_round_trip();
    test_line_counts();
    test_editing();
    test_many_lines_and_long_lines();
    CHECK_REPORT("edit_buffer");
}
