#pragma once

extern "C" {

struct timeval {
    long tv_sec;
    int tv_usec;
};

int gettimeofday(struct timeval* tv, void* tz);

}
