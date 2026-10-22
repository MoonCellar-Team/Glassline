#pragma once

#include <stddef.h>

#define RAND_MAX 0x7fffffff

extern "C" {
int rand(void);
int puts(const char*);
void* malloc(__SIZE_TYPE__);
void free(void* ptr);
}
