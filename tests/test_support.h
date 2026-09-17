#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>

static unsigned int checks;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition);        \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)

#endif
