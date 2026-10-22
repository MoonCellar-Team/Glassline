#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class LongClass : public midp::Class {
public:
    LongClass();
    virtual ~LongClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
