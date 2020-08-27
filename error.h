#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#define ERROR(code, ...) {        \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
    exit(code);                   \
}

#define DATA_ERROR(line, ...) {                 \
    fprintf(stderr, "error: [line %d] ", line); \
    fprintf(stderr, __VA_ARGS__);               \
    fprintf(stderr, "\n");                      \
    exit(EX_DATAERR);                           \
}

#endif // ERROR_H
