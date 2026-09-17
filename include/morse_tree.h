#ifndef MORSE_TREE_H
#define MORSE_TREE_H

#include <stdbool.h>
#include <stdio.h>

/* This project supports the 26 letters and 10 digits, whose codes use at most five signals. */
#define MORSE_MAX_CODE_LENGTH 5
#define MORSE_CODE_CAPACITY (MORSE_MAX_CODE_LENGTH + 1)

typedef struct {
    char code[MORSE_CODE_CAPACITY];
    char symbol;
} MorseEntry;

typedef struct MorseNode MorseTree;

/* The caller owns the returned tree. NULL indicates an allocation failure. */
MorseTree *morse_tree_create(void);
void morse_tree_destroy(MorseTree *tree);

/*
 * Insert a nonempty dot/dash code shorter than MORSE_CODE_CAPACITY.
 * Reinserting a code replaces its symbol, as in the original algorithm.
 * On failure, any created placeholders remain owned by the tree and can be destroyed.
 */
bool morse_tree_insert(MorseTree *tree, char symbol, const char *code);

/* A returned entry is borrowed until the tree is modified or destroyed. */
const MorseEntry *morse_tree_find(const MorseTree *tree, const char *code);

/* Write mapped nodes in root-left-right order; empty placeholders are omitted. */
bool morse_tree_print(const MorseTree *tree, FILE *output);

#endif
