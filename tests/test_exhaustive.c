#include "morse_decoder.h"
#include "morse_table.h"
#include "morse_tree.h"

#include "morse_reference.h"
#include "test_support.h"

#include <string.h>

static void check_mappings(const MorseTree *tree) {
    for (size_t i = 0; i < REFERENCE_COUNT; ++i) {
        const MorseEntry *entry = morse_tree_find(tree, reference[i].code);
        CHECK(entry != NULL);
        CHECK(entry->symbol == reference[i].symbol);
        CHECK(strcmp(entry->code, reference[i].code) == 0);
    }
}

static void test_all_short_codes(const MorseTree *tree) {
    size_t mapped = 0;
    size_t unmapped = 0;
    for (size_t length = 1; length <= MORSE_MAX_CODE_LENGTH; ++length) {
        for (unsigned int bits = 0; bits < (1U << length); ++bits) {
            char code[MORSE_CODE_CAPACITY];
            for (size_t position = 0; position < length; ++position) {
                code[position] = (bits & (1U << (length - position - 1))) == 0 ? '.' : '-';
            }
            code[length] = '\0';
            char expected = '\0';
            for (size_t i = 0; i < REFERENCE_COUNT; ++i) {
                if (strcmp(code, reference[i].code) == 0) {
                    expected = reference[i].symbol;
                }
            }
            const MorseEntry *entry = morse_tree_find(tree, code);
            CHECK((entry != NULL) == (expected != '\0'));
            char decoded[2];
            size_t token;
            MorseDecodeStatus status =
                morse_decode_line(tree, code, decoded, sizeof(decoded), &token);
            CHECK(status == (expected == '\0' ? MORSE_DECODE_UNKNOWN_TOKEN : MORSE_DECODE_OK));
            CHECK(expected == '\0' ? token == 1
                                   : decoded[0] == expected && decoded[1] == '\0' && token == 0);
            if (entry == NULL) {
                ++unmapped;
            } else {
                CHECK(entry->symbol == expected);
                ++mapped;
            }
        }
    }
    CHECK(mapped == 36);
    CHECK(unmapped == 26);
}

static void test_all_pairs(const MorseTree *tree) {
    for (size_t i = 0; i < REFERENCE_COUNT; ++i) {
        for (size_t j = 0; j < REFERENCE_COUNT; ++j) {
            for (unsigned int separated = 0; separated < 2; ++separated) {
                char encoded[32];
                CHECK(snprintf(encoded, sizeof(encoded), "%s %s%s", reference[i].code,
                               separated != 0 ? "/ " : "", reference[j].code) > 0);
                char expected[4] = {reference[i].symbol, reference[j].symbol, '\0', '\0'};
                if (separated != 0) {
                    expected[1] = ' ';
                    expected[2] = reference[j].symbol;
                }
                char decoded[4];
                CHECK(morse_decode_line(tree, encoded, decoded, sizeof(decoded), NULL) ==
                      MORSE_DECODE_OK);
                CHECK(strcmp(decoded, expected) == 0);
            }
        }
    }
}

static void check_preorder(const MorseTree *tree) {
    FILE *actual = tmpfile();
    FILE *expected = fopen("tests/fixtures/tree-only.expected", "r");
    CHECK(actual != NULL && expected != NULL);
    CHECK(morse_tree_print(tree, actual));
    rewind(actual);
    int byte;
    while ((byte = fgetc(expected)) != EOF) {
        CHECK(fgetc(actual) == byte);
    }
    CHECK(!ferror(expected));
    CHECK(fgetc(actual) == EOF && !ferror(actual));
    CHECK(fclose(actual) == 0);
    CHECK(fclose(expected) == 0);
}

static void test_insertion_orders(void) {
    const size_t strides[] = {1, 5, 7, 11, 13};
    for (size_t order = 0; order < sizeof(strides) / sizeof(strides[0]); ++order) {
        MorseTree *tree = morse_tree_create();
        CHECK(tree != NULL);
        for (size_t i = 0; i < REFERENCE_COUNT; ++i) {
            /* Each stride is coprime to 36, so every mapping is inserted exactly once. */
            size_t index = (REFERENCE_COUNT - 1 - i * strides[order] % REFERENCE_COUNT);
            CHECK(morse_tree_insert(tree, reference[index].symbol, reference[index].code));
        }
        check_mappings(tree);
        check_preorder(tree);
        morse_tree_destroy(tree);
    }
}

int main(void) {
    MorseTree *tree = morse_table_load("data/morse.txt", stderr);
    CHECK(tree != NULL);
    check_mappings(tree);
    test_all_short_codes(tree);
    test_all_pairs(tree);
    check_preorder(tree);
    test_insertion_orders();
    morse_tree_destroy(tree);
    printf("Exhaustive tests passed (%u checks; 62 codes and 2592 pair cases).\n", checks);
    return EXIT_SUCCESS;
}
