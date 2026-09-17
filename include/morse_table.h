#ifndef MORSE_TABLE_H
#define MORSE_TABLE_H

#include "morse_tree.h"

#include <stdio.h>

/*
 * Load a table from an explicit path. The caller owns the resulting tree.
 * Failure returns NULL, releases partial state, and writes an English diagnostic.
 * The diagnostics stream is borrowed and may be NULL to suppress messages.
 */
MorseTree *morse_table_load(const char *path, FILE *diagnostics);

#endif
