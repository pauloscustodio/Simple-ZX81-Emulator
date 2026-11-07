//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void die(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void* xmalloc(size_t size) {
    void* p = malloc(size);
    if (!p) {
        die("Out of memory\n");
    }
    return p;
}

int xstat(const char* path, struct stat* buf) {
    if (stat(path, buf) != 0) {
        die("Error getting file size: %s\n", path);
    }
    return 0;
}

FILE* xfopen(const char* path, const char* mode) {
    FILE* fp = fopen(path, mode);
    if (!fp) {
        die("Error opening file: %s\n", path);
    }
    return fp;
}

int xfclose(FILE* stream) {
    if (fclose(stream) != 0) {
        die("Error closing file\n");
    }
    return 0;
}

void xfread(void* ptr, size_t size, size_t count, FILE* stream) {
    size_t result = fread(ptr, size, count, stream);
    if (result != count) {
        die("Error reading file\n");
    }
}
