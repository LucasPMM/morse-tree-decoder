#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdio.h>

/* Run the file-backed decoder using borrowed streams; no stream is closed here. */
int application_run(int argc, char *const argv[], FILE *input, FILE *output, FILE *diagnostics);

#endif
