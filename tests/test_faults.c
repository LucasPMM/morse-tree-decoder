#define _GNU_SOURCE

#include "application.h"
#include "morse_table.h"
#include "morse_tree.h"
#include "text_io.h"

#include "test_support.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

/* Linker wrapping affects project calls, not glibc's internal stream allocations. */
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
void __real_free(void *pointer);
FILE *__real_fopen(const char *path, const char *mode);
int __real_fclose(FILE *stream);
int __real_fflush(FILE *stream);
int __real_fputc(int byte, FILE *stream);

static bool tracking;
static size_t allocation_calls;
static size_t fail_allocation;
static void *live[256];
static size_t live_count;

static bool allocation_fails(void) {
    if (!tracking) {
        return false;
    }
    ++allocation_calls;
    return fail_allocation != 0 && allocation_calls == fail_allocation;
}

static void track_pointer(void *pointer) {
    if (tracking && pointer != NULL) {
        CHECK(live_count < sizeof(live) / sizeof(live[0]));
        live[live_count++] = pointer;
    }
}

static void forget_pointer(void *pointer) {
    for (size_t i = 0; i < live_count; ++i) {
        if (live[i] == pointer) {
            live[i] = live[--live_count];
            return;
        }
    }
}

void *__wrap_malloc(size_t size) {
    void *pointer = allocation_fails() ? NULL : __real_malloc(size);
    track_pointer(pointer);
    return pointer;
}

void *__wrap_calloc(size_t count, size_t size) {
    void *pointer = allocation_fails() ? NULL : __real_calloc(count, size);
    track_pointer(pointer);
    return pointer;
}

void *__wrap_realloc(void *pointer, size_t size) {
    if (allocation_fails()) {
        return NULL;
    }
    /* Remove the old identity before realloc; restore it when the allocation fails. */
    bool was_tracked = false;
    for (size_t i = 0; i < live_count; ++i) {
        if (live[i] == pointer) {
            forget_pointer(pointer);
            was_tracked = true;
            break;
        }
    }
    void *result = __real_realloc(pointer, size);
    if (result != NULL) {
        track_pointer(result);
    } else if (was_tracked) {
        track_pointer(pointer);
    }
    return result;
}

void __wrap_free(void *pointer) {
    forget_pointer(pointer);
    __real_free(pointer);
}

static void start_tracking(size_t failure) {
    CHECK(live_count == 0);
    allocation_calls = 0;
    fail_allocation = failure;
    tracking = true;
}

static void stop_tracking(void) {
    CHECK(live_count == 0);
    tracking = false;
    fail_allocation = 0;
}

typedef struct {
    const char *text;
    size_t length;
    size_t position;
    size_t fail_after;
} ReadCookie;

static ssize_t fault_read(void *context, char *data, size_t size) {
    ReadCookie *cookie = context;
    if (cookie->position >= cookie->fail_after) {
        errno = EIO;
        return -1;
    }
    size_t available = cookie->length - cookie->position;
    size_t before_failure = cookie->fail_after - cookie->position;
    size_t count = available < size ? available : size;
    if (count > before_failure) {
        count = before_failure;
    }
    memcpy(data, cookie->text + cookie->position, count);
    cookie->position += count;
    return (ssize_t)count;
}

static FILE *fault_input(ReadCookie *cookie) {
    cookie_io_functions_t functions = {.read = fault_read};
    FILE *stream = fopencookie(cookie, "r", functions);
    CHECK(stream != NULL);
    return stream;
}

static ssize_t reject_write(void *context, const char *data, size_t size) {
    (void)context;
    (void)data;
    (void)size;
    errno = EIO;
    return 0;
}

static FILE *fault_output(void) {
    cookie_io_functions_t functions = {.write = reject_write};
    FILE *stream = fopencookie(NULL, "w", functions);
    CHECK(stream != NULL);
    CHECK(setvbuf(stream, NULL, _IONBF, 0) == 0);
    return stream;
}

static bool fail_open;
static bool fail_close;
static bool replace_table_input;
static ReadCookie table_cookie;
static FILE *table_file;
static FILE *output_target;
static bool fail_flush;
static bool fail_newline;

FILE *__wrap_fopen(const char *path, const char *mode) {
    if (fail_open) {
        errno = EACCES;
        return NULL;
    }
    table_file = replace_table_input ? fault_input(&table_cookie) : __real_fopen(path, mode);
    return table_file;
}

int __wrap_fclose(FILE *stream) {
    bool is_table = stream == table_file;
    if (is_table) {
        table_file = NULL;
    }
    int status = __real_fclose(stream);
    if (is_table && fail_close) {
        errno = EIO;
        return EOF;
    }
    return status;
}

int __wrap_fflush(FILE *stream) {
    if (fail_flush && stream == output_target) {
        errno = EIO;
        return EOF;
    }
    return __real_fflush(stream);
}

int __wrap_fputc(int byte, FILE *stream) {
    if (fail_newline && stream == output_target) {
        errno = EIO;
        return EOF;
    }
    return __real_fputc(byte, stream);
}

static FILE *input_with_text(const char *text) {
    FILE *input = tmpfile();
    CHECK(input != NULL);
    CHECK(fputs(text, input) >= 0);
    rewind(input);
    return input;
}

static void check_diagnostic(FILE *stream, const char *reason) {
    rewind(stream);
    char text[1024] = {0};
    CHECK(fread(text, 1, sizeof(text) - 1, stream) > 0);
    CHECK(strstr(text, reason) != NULL);
}

static size_t allocation_run(size_t failure) {
    FILE *input = input_with_text(".\n");
    CHECK(fseek(input, 0, SEEK_END) == 0);
    for (size_t i = 0; i < 2000; ++i) {
        if (fputs(". ", input) == EOF) {
            CHECK(false);
        }
    }
    CHECK(fputc('\n', input) != EOF);
    rewind(input);
    FILE *output = tmpfile();
    FILE *diagnostics = tmpfile();
    CHECK(output != NULL && diagnostics != NULL);
    char name[] = "morse";
    char *arguments[] = {name, NULL};
    start_tracking(failure);
    int status = application_run(1, arguments, input, output, diagnostics);
    CHECK(status == (failure == 0 ? EXIT_SUCCESS : EXIT_FAILURE));
    size_t calls = allocation_calls;
    if (failure != 0) {
        CHECK(calls >= failure);
        check_diagnostic(diagnostics, "allocat");
    }
    stop_tracking();
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
    CHECK(fclose(diagnostics) == 0);
    return calls;
}

static void test_allocation_failures(void) {
    size_t count = allocation_run(0);
    CHECK(count > 0);
    for (size_t failure = 1; failure <= count; ++failure) {
        (void)allocation_run(failure);
    }
    TextBuffer buffer = {0};
    start_tracking(1);
    CHECK(!text_buffer_reserve(&buffer, (size_t)PTRDIFF_MAX));
    CHECK(allocation_calls == 1);
    text_buffer_destroy(&buffer);
    stop_tracking();
    FILE *input = input_with_text("\n");
    start_tracking(1);
    CHECK(text_read_line(input, &buffer) == TEXT_LINE_ALLOCATION_ERROR);
    text_buffer_destroy(&buffer);
    stop_tracking();
    CHECK(fclose(input) == 0);
    printf("Allocation failure matrix passed (%zu allocation points).\n", count);
}

static void check_table_failure(const char *reason) {
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics != NULL);
    start_tracking(0);
    CHECK(morse_table_load("data/morse.txt", diagnostics) == NULL);
    check_diagnostic(diagnostics, reason);
    stop_tracking();
    CHECK(fclose(diagnostics) == 0);
}

static void test_table_io_failures(void) {
    fail_open = true;
    check_table_failure("file could not be opened");
    fail_open = false;
    replace_table_input = true;
    table_cookie = (ReadCookie){"E .\nT -\n", 8, 0, 4};
    check_table_failure("input read failed");
    replace_table_input = false;
    fail_close = true;
    check_table_failure("file close failed");
    fail_close = false;
}

static void check_application_io(FILE *input, FILE *output, const char *flag, const char *reason) {
    CHECK(input != NULL && output != NULL);
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics != NULL);
    char name[] = "morse";
    char option[32];
    char *arguments[] = {name, option, NULL};
    if (flag != NULL) {
        CHECK(strlen(flag) < sizeof(option));
        strcpy(option, flag);
    }
    start_tracking(0);
    CHECK(application_run(flag == NULL ? 1 : 2, arguments, input, output, diagnostics) ==
          EXIT_FAILURE);
    check_diagnostic(diagnostics, reason);
    stop_tracking();
    fail_flush = false;
    fail_newline = false;
    output_target = NULL;
    CHECK(fclose(input) == 0);
    /* A deliberately failed output stream may also report failure when closed. */
    (void)fclose(output);
    CHECK(fclose(diagnostics) == 0);
}

static void test_application_io_failures(void) {
    ReadCookie cookie = {".\nx", 3, 0, 2};
    check_application_io(fault_input(&cookie), tmpfile(), NULL, "input read failed");
    check_application_io(input_with_text(".\n"), fault_output(), NULL, "write decoded output");
    check_application_io(input_with_text(""), fault_output(), "-a", "write Morse tree");
    check_application_io(input_with_text(""), fault_output(), "--help", "command-line help");
    output_target = tmpfile();
    CHECK(output_target != NULL);
    fail_flush = true;
    check_application_io(input_with_text(".\n"), output_target, NULL, "flush output");
    output_target = tmpfile();
    CHECK(output_target != NULL);
    fail_newline = true;
    check_application_io(input_with_text(".\n"), output_target, NULL, "write decoded output");
}

static void test_right_subtree_output_failure(void) {
    start_tracking(0);
    MorseTree *tree = morse_tree_create();
    CHECK(tree != NULL);
    CHECK(morse_tree_insert(tree, 'T', "-"));
    FILE *output = fault_output();
    CHECK(!morse_tree_print(tree, output));
    morse_tree_destroy(tree);
    stop_tracking();
    (void)fclose(output);
}

int main(void) {
    test_allocation_failures();
    test_table_io_failures();
    test_application_io_failures();
    test_right_subtree_output_failure();
    printf("Fault-injection tests passed (%u checks).\n", checks);
    return EXIT_SUCCESS;
}
