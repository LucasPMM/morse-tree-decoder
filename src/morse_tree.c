#include "morse_tree.h"

#include <stdlib.h>
#include <string.h>

struct MorseNode {
    MorseEntry entry;
    struct MorseNode *left;
    struct MorseNode *right;
};

MorseTree *morse_tree_create(void) {
    /* Zero initialization makes placeholder keys valid strings and symbol zero unmapped. */
    return calloc(1, sizeof(MorseTree));
}

void morse_tree_destroy(MorseTree *tree) {
    if (tree != NULL) {
        morse_tree_destroy(tree->left);
        morse_tree_destroy(tree->right);
        free(tree);
    }
}

static bool valid_code(const char *code) {
    if (code == NULL) {
        return false;
    }
    size_t length = strlen(code);
    return length > 0 && length < MORSE_CODE_CAPACITY && strspn(code, ".-") == length;
}

static bool insert_entry(MorseTree *tree, const MorseEntry *entry, const char *signal) {
    if (*signal == '\0') {
        tree->entry = *entry;
        return true;
    }

    /* Preserve the original binary trie: dot goes left, dash goes right. */
    MorseTree **child = *signal == '.' ? &tree->left : &tree->right;
    if (*child == NULL) {
        *child = morse_tree_create();
        if (*child == NULL) {
            return false;
        }
    }
    return insert_entry(*child, entry, signal + 1);
}

bool morse_tree_insert(MorseTree *tree, char symbol, const char *code) {
    if (tree == NULL || symbol == '\0' || !valid_code(code)) {
        return false;
    }
    MorseEntry entry = {.symbol = symbol};
    memcpy(entry.code, code, strlen(code) + 1);
    return insert_entry(tree, &entry, code);
}

static const MorseEntry *find_entry(const MorseTree *tree, const char *signal) {
    if (tree == NULL) {
        return NULL;
    }
    if (*signal == '\0') {
        return tree->entry.symbol == '\0' ? NULL : &tree->entry;
    }
    const MorseTree *child = *signal == '.' ? tree->left : tree->right;
    return find_entry(child, signal + 1);
}

const MorseEntry *morse_tree_find(const MorseTree *tree, const char *code) {
    return valid_code(code) ? find_entry(tree, code) : NULL;
}

bool morse_tree_print(const MorseTree *tree, FILE *output) {
    if (output == NULL) {
        return false;
    }
    if (tree == NULL) {
        return true;
    }
    if (tree->entry.symbol != '\0' &&
        fprintf(output, "%c %s\n", tree->entry.symbol, tree->entry.code) < 0) {
        return false;
    }
    return morse_tree_print(tree->left, output) && morse_tree_print(tree->right, output);
}
