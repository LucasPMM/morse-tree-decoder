#include "application.h"
#include "morse_decoder.h"
#include "morse_table.h"
#include "morse_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int checks;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition);        \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)

static void test_tree(void) {
    MorseTree *tree = morse_tree_create();
    CHECK(tree != NULL);
    CHECK(morse_tree_find(tree, "") == NULL);
    CHECK(morse_tree_find(tree, ".") == NULL);
    CHECK(morse_tree_insert(tree, 'S', "..."));
    CHECK(morse_tree_find(tree, ".") == NULL);
    CHECK(morse_tree_find(tree, "..") == NULL);
    CHECK(morse_tree_insert(tree, 'E', "."));
    CHECK(morse_tree_insert(tree, 'T', "-"));
    CHECK(morse_tree_insert(tree, 'X', "..."));
    CHECK(morse_tree_find(tree, "...")->symbol == 'X');
    CHECK(strcmp(morse_tree_find(tree, "...")->code, "...") == 0);
    CHECK(morse_tree_find(tree, "-")->symbol == 'T');
    CHECK(morse_tree_find(tree, "--") == NULL);
    CHECK(morse_tree_find(tree, "x") == NULL);
    CHECK(morse_tree_find(tree, NULL) == NULL);
    CHECK(!morse_tree_insert(tree, 'Q', ""));
    CHECK(!morse_tree_insert(tree, 'Q', "x"));
    CHECK(!morse_tree_insert(tree, 'Q', ".........."));
    CHECK(!morse_tree_insert(tree, '\0', "."));
    CHECK(!morse_tree_insert(NULL, 'Q', "."));
    CHECK(!morse_tree_print(tree, NULL));

    FILE *output = tmpfile();
    CHECK(output != NULL);
    CHECK(morse_tree_print(tree, output));
    rewind(output);
    char buffer[64] = {0};
    size_t count = fread(buffer, 1, sizeof(buffer) - 1, output);
    CHECK(count == strlen("E .\nX ...\nT -\n"));
    CHECK(strcmp(buffer, "E .\nX ...\nT -\n") == 0);
    CHECK(fclose(output) == 0);
    morse_tree_destroy(tree);
    morse_tree_destroy(NULL);
}

static void test_table_and_decoder(void) {
    MorseTree *tree = morse_table_load("data/morse.txt", stderr);
    CHECK(tree != NULL);
    char decoded[128];
    size_t token = 99;
    CHECK(morse_decode_line(tree, "... --- ... / .... . .-.. .--.", decoded, sizeof(decoded),
                            &token) == MORSE_DECODE_OK);
    CHECK(strcmp(decoded, "SOS HELP") == 0);
    CHECK(token == 0);
    CHECK(morse_decode_line(tree, " /  ... / / --- / ", decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_OK);
    CHECK(strcmp(decoded, " S  O ") == 0);
    CHECK(morse_decode_line(tree, "", decoded, sizeof(decoded), NULL) == MORSE_DECODE_OK);
    CHECK(strcmp(decoded, "") == 0);
    CHECK(morse_decode_line(tree, "   ", decoded, 1, NULL) == MORSE_DECODE_OK);
    CHECK(morse_decode_line(tree, ".", decoded, 1, &token) == MORSE_DECODE_BUFFER_TOO_SMALL);
    CHECK(token == 1);
    CHECK(morse_decode_line(tree, ".", decoded, 2, NULL) == MORSE_DECODE_OK);
    CHECK(strcmp(decoded, "E") == 0);
    CHECK(morse_decode_line(tree, "... x", decoded, sizeof(decoded), &token) ==
          MORSE_DECODE_UNKNOWN_TOKEN);
    CHECK(token == 2);
    CHECK(morse_decode_line(tree, "..........", decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_UNKNOWN_TOKEN);
    CHECK(morse_decode_line(tree, ".../---", decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_UNKNOWN_TOKEN);
    CHECK(morse_decode_line(tree, "...\t---", decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_UNKNOWN_TOKEN);
    CHECK(morse_decode_line(tree, "", decoded, 0, NULL) == MORSE_DECODE_BUFFER_TOO_SMALL);
    CHECK(morse_decode_line(NULL, "", decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_INVALID_ARGUMENT);
    CHECK(morse_decode_line(tree, NULL, decoded, sizeof(decoded), NULL) ==
          MORSE_DECODE_INVALID_ARGUMENT);
    CHECK(morse_decode_line(tree, "", NULL, sizeof(decoded), NULL) ==
          MORSE_DECODE_INVALID_ARGUMENT);
    morse_tree_destroy(tree);

    tree = morse_table_load("tests/fixtures/historical/original-mapping-2019.txt", stderr);
    CHECK(tree != NULL);
    CHECK(morse_tree_find(tree, "..")->symbol == 'U');
    morse_tree_destroy(tree);
    CHECK(morse_table_load(NULL, NULL) == NULL);
    CHECK(morse_table_load("tests/fixtures/missing-table.txt", NULL) == NULL);
    CHECK(morse_table_load("tests/fixtures/invalid-table.txt", NULL) == NULL);
}

static void test_application_failure(void) {
    FILE *input = tmpfile();
    FILE *output = tmpfile();
    FILE *diagnostics = tmpfile();
    CHECK(input != NULL && output != NULL && diagnostics != NULL);
    CHECK(fputs("... x\n", input) >= 0);
    rewind(input);
    char name[] = "morse";
    char flag[] = "-a";
    char *arguments[] = {name, flag, NULL};
    CHECK(application_run(2, arguments, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(ftell(output) == 0);
    rewind(diagnostics);
    char message[128];
    CHECK(fgets(message, sizeof(message), diagnostics) != NULL);
    CHECK(strstr(message, "chunk 1 at token 2") != NULL);
    CHECK(strstr(message, "unknown or invalid Morse token") != NULL);
    CHECK(application_run(2, arguments, NULL, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(2, arguments, input, NULL, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(2, arguments, input, output, NULL) == EXIT_FAILURE);
    /* All streams are still owned by the caller, including after a failed decode. */
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
    CHECK(fclose(diagnostics) == 0);
}

int main(void) {
    test_tree();
    test_table_and_decoder();
    test_application_failure();
    printf("Module tests passed (%u checks).\n", checks);
    return EXIT_SUCCESS;
}
