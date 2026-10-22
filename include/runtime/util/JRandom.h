#pragma once

#include "runtime/midp/Object.h"

namespace midp {
class Class;
}

namespace util {

class JRandom : public midp::Object {
public:
    JRandom();
    virtual ~JRandom();
    virtual midp::Class* getClass() const;
    float nextFloat();
    int nextInt(int n);
    void setSeed(long long seed);
    int next(int bits);

    unsigned long long m_seed;
};

}
