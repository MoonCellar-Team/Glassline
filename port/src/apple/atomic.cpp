#include <stdint.h>

extern "C" int32_t OSAtomicAdd32(int32_t amount, volatile int32_t* value) {
    return __atomic_add_fetch(value, amount, __ATOMIC_SEQ_CST);
}
