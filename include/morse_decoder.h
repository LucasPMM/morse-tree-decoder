#ifndef MORSE_DECODER_H
#define MORSE_DECODER_H

#include "morse_tree.h"

#include <stddef.h>

typedef enum {
    MORSE_DECODE_OK,
    MORSE_DECODE_INVALID_ARGUMENT,
    MORSE_DECODE_UNKNOWN_TOKEN,
    MORSE_DECODE_BUFFER_TOO_SMALL
} MorseDecodeStatus;

/*
 * Decode one newline-free line without modifying it or performing stream I/O.
 * The caller owns the output buffer; its content is usable only on success.
 * error_token, when provided, receives the one-based failing token (zero on success).
 * Only spaces separate tokens in this compatibility stage. A standalone '/' emits a space.
 */
MorseDecodeStatus morse_decode_line(const MorseTree *tree, const char *line, char *decoded,
                                    size_t capacity, size_t *error_token);

#endif
