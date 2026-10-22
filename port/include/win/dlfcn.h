#pragma once

#define RTLD_LAZY 0x1
#define RTLD_NOW 0x2
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL 0
#define RTLD_DEFAULT ((void*)0)
#define RTLD_NEXT ((void*)-1)

#ifdef __cplusplus
extern "C" {
#endif
void* dlopen(const char* file, int mode);
void* dlsym(void* handle, const char* name);
int dlclose(void* handle);
char* dlerror(void);
#ifdef __cplusplus
}
#endif
