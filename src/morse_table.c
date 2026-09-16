#include "morse_table.h"

#include <stdbool.h>
#include <string.h>

#define TABLE_LINE_CAPACITY 128

static void report_failure(FILE *diagnostics, const char *path, size_t line_number,
                           const char *reason) {
    if (diagnostics != NULL) {
        fprintf(diagnostics, "Cannot load Morse table '%s' at line %zu: %s.\n", path, line_number,
                reason);
    }
}

static bool load_entries(FILE *file, MorseTree *tree, const char *path, FILE *diagnostics) {
    char line[TABLE_LINE_CAPACITY];
    size_t line_number = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        ++line_number;
        if (strspn(line, " \t\r\n") == strlen(line)) {
            continue;
        }
        MorseEntry entry = {0};
        char extra;
        /* Bound the field and reject trailing data instead of risking an unbounded %s. */
        if (sscanf(line, " %c %9s %c", &entry.symbol, entry.code, &extra) != 2) {
            report_failure(diagnostics, path, line_number, "expected one symbol and one code");
            return false;
        }
        if (!morse_tree_insert(tree, entry.symbol, entry.code)) {
            report_failure(diagnostics, path, line_number, "invalid code or allocation failure");
            return false;
        }
    }
    if (ferror(file)) {
        report_failure(diagnostics, path, line_number, "input read failed");
        return false;
    }
    return true;
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
