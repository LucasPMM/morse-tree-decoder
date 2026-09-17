#include "text_io.h"

#include <stdint.h>
#include <stdlib.h>

#define INITIAL_BUFFER_CAPACITY 128

bool text_buffer_reserve(TextBuffer *buffer, size_t required) {
    /* Bounding objects by PTRDIFF_MAX keeps subsequent length + 1/2 arithmetic safe. */
    if (buffer == NULL || required > (size_t)PTRDIFF_MAX) {
        return false;
    }
    if (required <= buffer->capacity) {
        return true;
    }
    size_t capacity = buffer->capacity == 0 ? INITIAL_BUFFER_CAPACITY : buffer->capacity;
    while (capacity < required) {
        if (capacity > (size_t)PTRDIFF_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    char *data = realloc(buffer->data, capacity);
    if (data == NULL) {
        return false;
    }
    buffer->data = data;
    buffer->capacity = capacity;
    return true;
}

void text_buffer_destroy(TextBuffer *buffer) {
    if (buffer != NULL) {
        free(buffer->data);
        *buffer = (TextBuffer){0};
    }
}

static TextLineStatus finish_line(TextBuffer *buffer, bool has_newline) {
    if (has_newline && buffer->length > 0 && buffer->data[buffer->length - 1] == '\r') {
        --buffer->length;
    }
    if (!text_buffer_reserve(buffer, buffer->length + 1)) {
        return TEXT_LINE_ALLOCATION_ERROR;
    }
    buffer->data[buffer->length] = '\0';
    return TEXT_LINE_OK;
}

TextLineStatus text_read_line(FILE *input, TextBuffer *buffer) {
    if (input == NULL || buffer == NULL) {
        return TEXT_LINE_INVALID_ARGUMENT;
    }
    buffer->length = 0;
    int byte;
    while ((byte = fgetc(input)) != EOF) {
        if (byte == '\n') {
            return finish_line(buffer, true);
        }
        if (byte > 126 || (byte < 32 && byte != '\t' && byte != '\r')) {
            return TEXT_LINE_INVALID_BYTE;
        }
        if (!text_buffer_reserve(buffer, buffer->length + 2)) {
            return TEXT_LINE_ALLOCATION_ERROR;
        }
        buffer->data[buffer->length++] = (char)byte;
    }
    if (ferror(input)) {
        return TEXT_LINE_READ_ERROR;
    }
    return buffer->length == 0 ? TEXT_LINE_EOF : finish_line(buffer, false);
}

const char *text_line_error(TextLineStatus status) {
    switch (status) {
    case TEXT_LINE_READ_ERROR:
        return "input read failed";
    case TEXT_LINE_ALLOCATION_ERROR:
        return "buffer allocation failed";
    case TEXT_LINE_INVALID_BYTE:
        return "unsupported byte in input";
    case TEXT_LINE_INVALID_ARGUMENT:
        return "invalid input stream or buffer";
    case TEXT_LINE_OK:
    case TEXT_LINE_EOF:
        return "no input error";
    }
    return "unknown input status";
}
