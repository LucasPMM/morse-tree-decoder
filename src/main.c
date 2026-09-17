#include "application.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
    return application_run(argc, argv, stdin, stdout, stderr);
}
