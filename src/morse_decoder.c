#include "morse_decoder.h"

#include <string.h>

static bool decode_token(const MorseTree *tree, const char *token, size_t length, char *symbol) {
    if (length == 1 && *token == '/') {
        *symbol = ' ';
        return true;
    }
    if (length >= MORSE_CODE_CAPACITY) {
        return false;
    }
    char code[MORSE_CODE_CAPACITY];
    memcpy(code, token, length);
    code[length] = '\0';
    const MorseEntry *entry = morse_tree_find(tree, code);
    if (entry == NULL) {
        return false;
    }
    *symbol = entry->symbol;
    return true;
}

MorseDecodeStatus morse_decode_line(const MorseTree *tree, const char *line, char *decoded,
                                    size_t capacity, size_t *error_token) {
    if (error_token != NULL) {
        *error_token = 0;
    }
    if (tree == NULL || line == NULL || decoded == NULL) {
        return MORSE_DECODE_INVALID_ARGUMENT;
    }
    if (capacity == 0) {
        return MORSE_DECODE_BUFFER_TOO_SMALL;
    }

    size_t used = 0;
    size_t token_index = 0;
    const char *cursor = line;
    while (*cursor != '\0') {
        cursor += strspn(cursor, " ");
        if (*cursor == '\0') {
            break;
        }
        size_t length = strcspn(cursor, " ");
        char symbol;
        ++token_index;
        if (error_token != NULL) {
            *error_token = token_index;
        }
        if (!decode_token(tree, cursor, length, &symbol)) {
            return MORSE_DECODE_UNKNOWN_TOKEN;
        }
        if (used >= capacity - 1) {
            return MORSE_DECODE_BUFFER_TOO_SMALL;
        }
        decoded[used++] = symbol;
        cursor += length;
    }
    decoded[used] = '\0';
    if (error_token != NULL) {
        *error_token = 0;
    }
    return MORSE_DECODE_OK;
}
