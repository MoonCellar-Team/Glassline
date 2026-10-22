#pragma once

#include <stdint.h>

extern "C" int32_t OSAtomicAdd32(int32_t amount, volatile int32_t* value);

inline int32_t OSAtomicIncrement32(volatile int32_t* value) { return OSAtomicAdd32(1, value); }
inline int32_t OSAtomicDecrement32(volatile int32_t* value) { return OSAtomicAdd32(-1, value); }
