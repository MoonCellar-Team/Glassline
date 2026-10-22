#pragma once

#include <direct.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
inline int mkdir(const char* path, int) { return _mkdir(path); }
extern "C" {
#endif
ssize_t readlink(const char* path, char* buf, size_t size);
int medecomp_gethostname(char* name, size_t size);
#ifdef __cplusplus
}
#endif

#define getc_unlocked _getc_nolock
#define flockfile _lock_file
#define funlockfile _unlock_file
