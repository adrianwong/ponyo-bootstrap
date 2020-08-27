#include <stdio.h>
#include <stdlib.h>

#include "error.h"

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

    // Just print contents of buffer for now.
    printf("%s", source);

    free(source);
}

int main(int argc, const char** argv) {
    if (argc != 2) {
        ERROR(EX_USAGE, "usage: %s [path/to/file]", argv[0]);
    }

    run_file(argv[1]);

    return EX_OK;
}
