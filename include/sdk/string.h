#pragma once

#include <stddef.h>

extern "C" {
int strncmp(const char* a, const char* b, size_t n);
void* memset(void* dst, int value, size_t n);
void* memmove(void* dst, const void* src, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
}
