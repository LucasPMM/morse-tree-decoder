#include "application.h"

#include "morse_decoder.h"
#include "morse_table.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef MORSE_TABLE_PATH
#define MORSE_TABLE_PATH "data/morse.txt"
#endif

/* Fixed input chunks are retained for compatibility; dynamic physical lines follow in phase 4. */
#define MESSAGE_CAPACITY 500

static const char *decode_failure_reason(MorseDecodeStatus status) {
    switch (status) {
    case MORSE_DECODE_INVALID_ARGUMENT:
        return "invalid decoder argument";
    case MORSE_DECODE_UNKNOWN_TOKEN:
        return "unknown or invalid Morse token";
    case MORSE_DECODE_BUFFER_TOO_SMALL:
        return "decoded message exceeds buffer capacity";
    case MORSE_DECODE_OK:
        return "no decoding error";
    }
    return "unknown decoder status";
}

static bool decode_stream(const MorseTree *tree, FILE *input, FILE *output, FILE *diagnostics) {
    char encoded[MESSAGE_CAPACITY];
    char decoded[MESSAGE_CAPACITY];
    size_t line_number = 0;
    while (fgets(encoded, sizeof(encoded), input) != NULL) {
        size_t length = strlen(encoded);
        if (length > 0 && encoded[length - 1] == '\n') {
            encoded[length - 1] = '\0';
        }
        size_t error_token;
        MorseDecodeStatus status =
            morse_decode_line(tree, encoded, decoded, sizeof(decoded), &error_token);
        if (status != MORSE_DECODE_OK) {
            fprintf(diagnostics, "Cannot decode input chunk %zu at token %zu: %s.\n",
                    line_number + 1, error_token, decode_failure_reason(status));
            return false;
        }
        /* Preserve legacy separators and missing final newline until the explicit format fix. */
        if ((line_number > 0 && fputc('\n', output) == EOF) || fputs(decoded, output) == EOF) {
            fprintf(diagnostics, "Cannot write decoded output.\n");
            return false;
        }
        ++line_number;
    }
    if (ferror(input)) {
        fprintf(diagnostics, "Cannot read encoded input.\n");
        return false;
    }
    return true;
}

int application_run(int argc, char *const argv[], FILE *input, FILE *output, FILE *diagnostics) {
    if (input == NULL || output == NULL || diagnostics == NULL) {
        return EXIT_FAILURE;
    }
    /* Strict option validation and long aliases belong to the phase 4 behavior change. */
    bool print_tree = argc == 2 && strcmp(argv[1], "-a") == 0;
    MorseTree *tree = morse_table_load(MORSE_TABLE_PATH, diagnostics);
    if (tree == NULL) {
        return EXIT_FAILURE;
    }
    bool success = decode_stream(tree, input, output, diagnostics);
    if (success && print_tree && !morse_tree_print(tree, output)) {
        fprintf(diagnostics, "Cannot write Morse tree.\n");
        success = false;
    }
    if (fflush(output) == EOF) {
        fprintf(diagnostics, "Cannot flush output.\n");
        success = false;
    }
    morse_tree_destroy(tree);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
