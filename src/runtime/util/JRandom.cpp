#include "runtime/util/JRandom.h"

#include "runtime/midp/System.h"
#include "runtime/util/RandomClass.h"

namespace util {

midp::Class* JRandom::getClass() const {
    return new RandomClass();
}

float JRandom::nextFloat() {
    return (float)next(24) * (1.0f / 16777216.0f);
}

int JRandom::nextInt(int n) {
    unsigned u = (unsigned)n;
    if ((u & (0u - u)) == u) {
        long long product = (long long)next(31) * (long long)n;
        return (int)(product >> 31);
    }
    for (;;) {
        int bits = next(31);
        int val = bits % n;
        int span = (int)((unsigned)(n - 1) + (unsigned)bits - (unsigned)val);
        if (span >= 0)
            return val;
    }
}

void JRandom::setSeed(long long seed) {
    m_seed = ((unsigned long long)seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1);
}

JRandom::~JRandom() {}

JRandom::JRandom() : m_seed(0) {
    setSeed(midp::System::currentTimeMillis());
}

int JRandom::next(int bits) {
    unsigned long long nextSeed = (m_seed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1);
    m_seed = nextSeed;
    return (int)(nextSeed >> (48 - bits));
}

}
