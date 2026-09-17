#include "morse_table.h"

#include "text_io.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>

static void report_failure(FILE *diagnostics, const char *path, size_t line_number,
                           const char *reason) {
    if (diagnostics != NULL) {
        fprintf(diagnostics, "Cannot load Morse table '%s' at line %zu: %s.\n", path, line_number,
                reason);
    }
}

static bool parse_entry(const char *line, MorseEntry *entry) {
    const char *cursor = line + strspn(line, " \t");
    entry->symbol = *cursor++;
    if (!((entry->symbol >= 'A' && entry->symbol <= 'Z') ||
          (entry->symbol >= '0' && entry->symbol <= '9')) ||
        (*cursor != ' ' && *cursor != '\t')) {
        return false;
    }
    cursor += strspn(cursor, " \t");
    size_t length = strcspn(cursor, " \t");
    if (length == 0 || length > MORSE_MAX_CODE_LENGTH || strspn(cursor, ".-") != length) {
        return false;
    }
    const char *end = cursor + length;
    if (*(end + strspn(end, " \t")) != '\0') {
        return false;
    }
    memcpy(entry->code, cursor, length);
    entry->code[length] = '\0';
    return true;
}

static bool load_entries(FILE *file, MorseTree *tree, const char *path, FILE *diagnostics) {
    TextBuffer line = {0};
    /* File uniqueness is stricter than the low-level trie's intentional replacement behavior. */
    bool seen_symbols[UCHAR_MAX + 1] = {false};
    size_t line_number = 0;
    size_t entry_count = 0;
    bool success = true;
    TextLineStatus status;
    while ((status = text_read_line(file, &line)) == TEXT_LINE_OK) {
        ++line_number;
        if (strspn(line.data, " \t") == line.length) {
            continue;
        }
        MorseEntry entry = {0};
        const char *reason = NULL;
        if (!parse_entry(line.data, &entry)) {
            reason = "expected one uppercase letter or digit and a 1-5 signal code";
        } else if (seen_symbols[(unsigned char)entry.symbol]) {
            reason = "duplicate symbol";
        } else if (morse_tree_find(tree, entry.code) != NULL) {
            reason = "duplicate code";
        } else if (!morse_tree_insert(tree, entry.symbol, entry.code)) {
            reason = "tree allocation failed";
        }
        if (reason != NULL) {
            report_failure(diagnostics, path, line_number, reason);
            success = false;
            break;
        }
        seen_symbols[(unsigned char)entry.symbol] = true;
        ++entry_count;
    }
    if (success && status != TEXT_LINE_EOF) {
        report_failure(diagnostics, path, line_number + 1, text_line_error(status));
        success = false;
    } else if (success && entry_count == 0) {
        report_failure(diagnostics, path, line_number, "table contains no mappings");
        success = false;
    }
    text_buffer_destroy(&line);
    return success;
}

MorseTree *morse_table_load(const char *path, FILE *diagnostics) {
    if (path == NULL) {
        report_failure(diagnostics, "(null)", 0, "invalid table path");
        return NULL;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        report_failure(diagnostics, path, 0, "file could not be opened");
        return NULL;
    }
    MorseTree *tree = morse_tree_create();
    bool success = tree != NULL;
    if (success) {
        success = load_entries(file, tree, path, diagnostics);
    } else {
        report_failure(diagnostics, path, 0, "allocation failed");
    }
    if (fclose(file) != 0) {
        report_failure(diagnostics, path, 0, "file close failed");
        success = false;
    }
    if (!success) {
        morse_tree_destroy(tree);
        return NULL;
    }
    return tree;
}
