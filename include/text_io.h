#ifndef TEXT_IO_H
#define TEXT_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextBuffer;

typedef enum {
    TEXT_LINE_OK,
    TEXT_LINE_EOF,
    TEXT_LINE_READ_ERROR,
    TEXT_LINE_ALLOCATION_ERROR,
    TEXT_LINE_INVALID_BYTE,
    TEXT_LINE_INVALID_ARGUMENT
} TextLineStatus;

/* Initialize buffers with {0}. They own their data, including after a failed operation. */
bool text_buffer_reserve(TextBuffer *buffer, size_t required);
void text_buffer_destroy(TextBuffer *buffer);

/*
 * Read a complete ASCII line from a borrowed stream, removing LF or CRLF.
 * EOF without a final newline is accepted. NUL, DEL, non-ASCII, and unsupported
 * controls fail immediately; an internal or unterminated CR is left for the parser to reject.
 * data/length describe a NUL-terminated line only on TEXT_LINE_OK.
 */
TextLineStatus text_read_line(FILE *input, TextBuffer *buffer);
const char *text_line_error(TextLineStatus status);

#endif
