#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "eval.h"
#include "lexer.h"
#include "parser.h"

static void interpret(const char* source) {
    init_lexer(source);

    for (;;) {
        Expr* e = parse_expr();
        if (e != NULL) {
            print_expr(eval(e));
        } else {
            return;
        }
    }
}

#ifndef PONYO_TEST
static char* read_file(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        ERROR(EX_IOERR, "error: could not open '%s'", file_path);
    }

    // Get file size.
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    // Allocate buffer of size `file_size + 1` (+1 for null terminator).
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        ERROR(EX_IOERR, "error: not enough memory to read '%s'", file_path);
    }

    // Read file into buffer.
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        ERROR(EX_IOERR, "error: could not read '%s'", file_path);
    }
    buffer[bytes_read] = '\0';

    fclose(file);

    return buffer;
}

static void run_file(const char* file_path) {
    char* source = read_file(file_path);

    interpret(source);

    free(source);
}
#else
static void run_test(void) {
    char buffer[1024] = { '\0' };
    int used = 0;
    int left = sizeof(buffer);

    while (!feof(stdin)) {
        fgets(buffer + used, left, stdin);

        int length = strlen(buffer + used);
        used += length;
        left -= length;

        // fgets reads up to (left - 1) characters, so this will never be 0.
        if (left == 1) { break; }
    }

    interpret(buffer);
}
#endif

int main(int argc, const char** argv) {
#ifndef PONYO_TEST
    if (argc != 2) {
        ERROR(EX_USAGE, "usage: %s [path/to/file]", argv[0]);
    }

    run_file(argv[1]);
#else
    run_test();
#endif

    return EX_OK;
}
