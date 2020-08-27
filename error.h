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

#endif // ERROR_H
