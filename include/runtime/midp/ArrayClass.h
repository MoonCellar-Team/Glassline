#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ArrayClass : public midp::Class {
public:
    ArrayClass();
    virtual ~ArrayClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
