#include "application.h"
#include "morse_decoder.h"
#include "morse_table.h"
#include "morse_tree.h"
#include "test_support.h"
#include "text_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *fixture_path;

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
    CHECK(morse_decode_line(tree, "...\t---", decoded, sizeof(decoded), NULL) == MORSE_DECODE_OK);
    CHECK(strcmp(decoded, "SO") == 0);
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
    CHECK(strstr(message, "line 1 at token 2") != NULL);
    CHECK(strstr(message, "unknown or invalid Morse token") != NULL);
    CHECK(application_run(2, arguments, NULL, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(2, arguments, input, NULL, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(2, arguments, input, output, NULL) == EXIT_FAILURE);
    /* All streams are still owned by the caller, including after a failed decode. */
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
    CHECK(fclose(diagnostics) == 0);
}

static FILE *stream_with_bytes(const char *bytes, size_t length) {
    FILE *stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(bytes, 1, length, stream) == length);
    rewind(stream);
    return stream;
}

static void test_text_buffers(void) {
    TextBuffer buffer = {0};
    CHECK(!text_buffer_reserve(NULL, 1));
    CHECK(!text_buffer_reserve(&buffer, SIZE_MAX));
    CHECK(text_buffer_reserve(&buffer, 0));
    CHECK(buffer.data == NULL);
    CHECK(text_buffer_reserve(&buffer, 1));
    CHECK(buffer.capacity == 128);
    char *data = buffer.data;
    CHECK(text_buffer_reserve(&buffer, 128));
    CHECK(buffer.data == data);
    CHECK(text_buffer_reserve(&buffer, 129));
    CHECK(buffer.capacity == 256);
    CHECK(text_buffer_reserve(&buffer, 2049));
    CHECK(buffer.capacity >= 2049);
    text_buffer_destroy(&buffer);
    CHECK(buffer.data == NULL && buffer.capacity == 0 && buffer.length == 0);
    text_buffer_destroy(NULL);
    CHECK(strcmp(text_line_error(TEXT_LINE_READ_ERROR), "input read failed") == 0);
    CHECK(strcmp(text_line_error(TEXT_LINE_ALLOCATION_ERROR), "buffer allocation failed") == 0);
    CHECK(strcmp(text_line_error(TEXT_LINE_INVALID_BYTE), "unsupported byte in input") == 0);
    CHECK(strcmp(text_line_error(TEXT_LINE_INVALID_ARGUMENT), "invalid input stream or buffer") ==
          0);
    CHECK(strcmp(text_line_error(TEXT_LINE_OK), "no input error") == 0);
    CHECK(strcmp(text_line_error(TEXT_LINE_EOF), "no input error") == 0);
    CHECK(strcmp(text_line_error((TextLineStatus)999), "unknown input status") == 0);
}

static void test_line_reading(void) {
    TextBuffer buffer = {0};
    FILE *stream = stream_with_bytes("\n.\r\n..\n...", sizeof("\n.\r\n..\n...") - 1);
    CHECK(text_read_line(NULL, &buffer) == TEXT_LINE_INVALID_ARGUMENT);
    CHECK(text_read_line(stream, NULL) == TEXT_LINE_INVALID_ARGUMENT);
    const char *expected[] = {"", ".", "..", "..."};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        CHECK(text_read_line(stream, &buffer) == TEXT_LINE_OK);
        CHECK(strcmp(buffer.data, expected[i]) == 0);
        CHECK(buffer.length == strlen(expected[i]));
    }
    CHECK(text_read_line(stream, &buffer) == TEXT_LINE_EOF);
    CHECK(fclose(stream) == 0);
    const char invalid[] = {'\0', '\1', '\177', (char)255};
    for (size_t i = 0; i < sizeof(invalid); ++i) {
        stream = stream_with_bytes(&invalid[i], 1);
        CHECK(text_read_line(stream, &buffer) == TEXT_LINE_INVALID_BYTE);
        CHECK(fclose(stream) == 0);
    }
    text_buffer_destroy(&buffer);
}

typedef struct {
    const char *text;
    size_t length;
    bool valid;
    const char *reason;
} TableCase;

#define TABLE_CASE(text, valid, reason)                                                            \
    { text, sizeof(text) - 1, valid, reason }

static void check_table_case(const TableCase *test) {
    FILE *file = fopen(fixture_path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(test->text, 1, test->length, file) == test->length);
    CHECK(fclose(file) == 0);
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics != NULL);
    MorseTree *tree = morse_table_load(fixture_path, diagnostics);
    CHECK((tree != NULL) == test->valid);
    rewind(diagnostics);
    char message[512] = {0};
    size_t length = fread(message, 1, sizeof(message) - 1, diagnostics);
    if (test->valid) {
        CHECK(length == 0);
        CHECK(morse_tree_find(tree, ".")->symbol == 'E');
    } else {
        CHECK(length > 0);
        CHECK(strstr(message, test->reason) != NULL);
    }
    morse_tree_destroy(tree);
    CHECK(fclose(diagnostics) == 0);
    CHECK(remove(fixture_path) == 0);
}

static void test_mapping_validation(void) {
    const TableCase cases[] = {TABLE_CASE("E .\n", true, ""),
                               TABLE_CASE(" \tE\t.\t\r\n\n T -", true, ""),
                               TABLE_CASE("E .", true, ""),
                               TABLE_CASE("", false, "no mappings"),
                               TABLE_CASE("\n\t\r\n", false, "no mappings"),
                               TABLE_CASE("E\n", false, "expected one uppercase"),
                               TABLE_CASE("E \n", false, "expected one uppercase"),
                               TABLE_CASE("E.\n", false, "expected one uppercase"),
                               TABLE_CASE("EE .\n", false, "expected one uppercase"),
                               TABLE_CASE("a .\n", false, "expected one uppercase"),
                               TABLE_CASE("! .\n", false, "expected one uppercase"),
                               TABLE_CASE("E /\n", false, "expected one uppercase"),
                               TABLE_CASE("E x\n", false, "expected one uppercase"),
                               TABLE_CASE("E ......\n", false, "expected one uppercase"),
                               TABLE_CASE("E . extra\n", false, "expected one uppercase"),
                               TABLE_CASE("E . # comment\n", false, "expected one uppercase"),
                               TABLE_CASE("E .\nE -\n", false, "duplicate symbol"),
                               TABLE_CASE("E .\nT .\n", false, "duplicate code"),
                               TABLE_CASE("E .\r", false, "expected one uppercase"),
                               TABLE_CASE("E .\rX\n", false, "expected one uppercase"),
                               TABLE_CASE("E .\0\n", false, "unsupported byte"),
                               TABLE_CASE("E .\1\n", false, "unsupported byte"),
                               TABLE_CASE("E .\177\n", false, "unsupported byte"),
                               TABLE_CASE("E .\377\n", false, "unsupported byte")};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        check_table_case(&cases[i]);
    }
}

static void test_large_mapping_lines(void) {
    char padding[20001];
    memset(padding, ' ', sizeof(padding) - 1);
    padding[sizeof(padding) - 1] = '\0';
    FILE *file = fopen(fixture_path, "w");
    CHECK(file != NULL);
    CHECK(fputs(padding, file) >= 0);
    CHECK(fputs("E .\n", file) >= 0);
    CHECK(fclose(file) == 0);
    MorseTree *tree = morse_table_load(fixture_path, stderr);
    CHECK(tree != NULL);
    CHECK(morse_tree_find(tree, ".")->symbol == 'E');
    morse_tree_destroy(tree);
    CHECK(remove(fixture_path) == 0);
    memset(padding, '.', sizeof(padding) - 1);
    file = fopen(fixture_path, "w");
    CHECK(file != NULL);
    CHECK(fputs("E ", file) >= 0 && fputs(padding, file) >= 0);
    CHECK(fclose(file) == 0);
    CHECK(morse_table_load(fixture_path, NULL) == NULL);
    CHECK(remove(fixture_path) == 0);
}

static void test_invalid_application_arguments(void) {
    FILE *input = tmpfile();
    FILE *output = tmpfile();
    FILE *diagnostics = tmpfile();
    CHECK(input != NULL && output != NULL && diagnostics != NULL);
    char name[] = "morse";
    char *arguments[] = {name, NULL};
    char *missing_name[] = {NULL, NULL};
    CHECK(application_run(0, arguments, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(-1, arguments, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(1, NULL, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(1, missing_name, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(application_run(2, arguments, input, output, diagnostics) == EXIT_FAILURE);
    CHECK(ftell(output) == 0);
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
    CHECK(fclose(diagnostics) == 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: module-tests GENERATED_TABLE_PATH\n");
        return EXIT_FAILURE;
    }
    fixture_path = argv[1];
    test_tree();
    test_table_and_decoder();
    test_application_failure();
    test_invalid_application_arguments();
    test_text_buffers();
    test_line_reading();
    test_mapping_validation();
    test_large_mapping_lines();
    printf("Module tests passed (%u checks).\n", checks);
    return EXIT_SUCCESS;
}
