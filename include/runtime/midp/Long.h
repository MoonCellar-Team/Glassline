#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Class;

class Long : public Object {
public:
    explicit Long(long long value);
    virtual ~Long();
    virtual Class* getClass() const;

    long long m_value;
};

}
