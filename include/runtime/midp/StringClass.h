#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class StringClass : public midp::Class {
public:
    StringClass();
    virtual ~StringClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
