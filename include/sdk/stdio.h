#pragma once

#include <stddef.h>

extern "C" {
struct FILE;
typedef struct FILE FILE;

int snprintf(char* buf, size_t size, const char* format, ...);
FILE* fopen(const char* path, const char* mode);
int fclose(FILE* file);
int fseek(FILE* file, long offset, int whence);
long ftell(FILE* file);
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* file);
int puts(const char* s);
}
