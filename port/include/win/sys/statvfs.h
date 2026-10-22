#pragma once

struct statvfs {
    unsigned long long f_bsize, f_frsize, f_blocks, f_bfree, f_bavail;
};

#ifdef __cplusplus
extern "C"
#endif
int statvfs(const char* path, struct statvfs* out);
