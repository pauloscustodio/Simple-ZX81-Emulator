//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <sys/stat.h>

#define release_assert(x) do { \
    if (!(x)) die("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #x); \
} while (0)

void die(const char* msg, ...);
void* xmalloc(size_t size);
int xstat(const char* path, struct stat* buf);
FILE* xfopen(const char* path, const char* mode);
int xfclose(FILE* stream);
void xfread(void* ptr, size_t size, size_t count, FILE* stream);
