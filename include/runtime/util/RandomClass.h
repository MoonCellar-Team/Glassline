#pragma once

#include "runtime/midp/Class.h"

namespace util {

class RandomClass : public midp::Class {
public:
    RandomClass();
    virtual ~RandomClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
