#include "application.h"

#include "morse_decoder.h"
#include "morse_table.h"
#include "text_io.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef MORSE_TABLE_PATH
#define MORSE_TABLE_PATH "data/morse.txt"
#endif

static const char HELP_TEXT[] =
    "Usage: morse [-a|--print-tree] < input.txt\n"
    "       morse --help\n\n"
    "Decode uppercase letters and digits using " MORSE_TABLE_PATH ".\n"
    "Separate codes with spaces or tabs; use '/' as a word separator.\n\n"
    "  -a, --print-tree  Print mapped tree nodes in preorder after decoding.\n"
    "  --help           Show this help and exit.\n";

static bool parse_options(int argc, char *const argv[], bool *print_tree, bool *help,
                          FILE *diagnostics) {
    *print_tree = false;
    *help = false;
    if (argc < 1 || argv == NULL || argv[0] == NULL || (argc == 2 && argv[1] == NULL)) {
        fprintf(diagnostics, "Invalid command-line arguments.\n");
        return false;
    }
    if (argc > 2) {
        fprintf(diagnostics, "Expected at most one option. Run morse --help for usage.\n");
        return false;
    }
    if (argc == 1) {
        return true;
    }
    if (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "--print-tree") == 0) {
        *print_tree = true;
    } else if (strcmp(argv[1], "--help") == 0) {
        *help = true;
    } else {
        fprintf(diagnostics, "Unknown option '%s'. Run morse --help for usage.\n", argv[1]);
        return false;
    }
    return true;
}

static bool decode_message(const MorseTree *tree, const TextBuffer *encoded, TextBuffer *decoded,
                           size_t line_number, FILE *diagnostics) {
    /* A decoded character consumes at least one input byte, so this bound suffices. */
    if (!text_buffer_reserve(decoded, encoded->length + 1)) {
        fprintf(diagnostics, "Cannot allocate decoded output for line %zu.\n", line_number);
        return false;
    }
    size_t token;
    MorseDecodeStatus status =
        morse_decode_line(tree, encoded->data, decoded->data, decoded->capacity, &token);
    if (status != MORSE_DECODE_OK) {
        fprintf(diagnostics,
                "Cannot decode line %zu at token %zu: unknown or invalid Morse token.\n",
                line_number, token);
        return false;
    }
    return true;
}

static bool decode_stream(const MorseTree *tree, FILE *input, FILE *output, FILE *diagnostics) {
    TextBuffer encoded = {0};
    TextBuffer decoded = {0};
    size_t line_number = 0;
    bool success = true;
    TextLineStatus status;
    while ((status = text_read_line(input, &encoded)) == TEXT_LINE_OK) {
        ++line_number;
        if (!decode_message(tree, &encoded, &decoded, line_number, diagnostics)) {
            success = false;
            break;
        }
        if (fputs(decoded.data, output) == EOF || fputc('\n', output) == EOF) {
            fprintf(diagnostics, "Cannot write decoded output at line %zu.\n", line_number);
            success = false;
            break;
        }
    }
    if (success && status != TEXT_LINE_EOF) {
        fprintf(diagnostics, "Cannot read line %zu: %s.\n", line_number + 1,
                text_line_error(status));
        success = false;
    }
    text_buffer_destroy(&decoded);
    text_buffer_destroy(&encoded);
    return success;
}

int application_run(int argc, char *const argv[], FILE *input, FILE *output, FILE *diagnostics) {
    if (input == NULL || output == NULL || diagnostics == NULL) {
        return EXIT_FAILURE;
    }
    bool print_tree;
    bool help;
    if (!parse_options(argc, argv, &print_tree, &help, diagnostics)) {
        return EXIT_FAILURE;
    }
    bool success;
    if (help) {
        success = fputs(HELP_TEXT, output) != EOF;
    } else {
        MorseTree *tree = morse_table_load(MORSE_TABLE_PATH, diagnostics);
        if (tree == NULL) {
            return EXIT_FAILURE;
        }
        success = decode_stream(tree, input, output, diagnostics);
        if (success && print_tree && !morse_tree_print(tree, output)) {
            fprintf(diagnostics, "Cannot write Morse tree.\n");
            success = false;
        }
        morse_tree_destroy(tree);
    }
    if (fflush(output) == EOF) {
        fprintf(diagnostics, "Cannot flush output.\n");
        success = false;
    } else if (!success && help) {
        fprintf(diagnostics, "Cannot write command-line help.\n");
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
