#pragma once

#define _CTYPE_D 0x00000400L

extern "C" {
typedef struct {
    char __magic[8];
    char __encoding[32];
    void* __sgetrune;
    void* __sputrune;
    int __invalid_rune;
    unsigned int __runetype[256];
} _RuneLocale;

extern _RuneLocale _DefaultRuneLocale;
int __maskrune(int c, unsigned long f);
}

static inline int isascii(int c) {
    return (c & ~0x7F) == 0;
}

static inline int __istype(int c, unsigned long f) {
    return isascii(c) ? !!(_DefaultRuneLocale.__runetype[c] & f) : !!__maskrune(c, f);
}

static inline int isnumber(int c) {
    return __istype(c, _CTYPE_D);
}
