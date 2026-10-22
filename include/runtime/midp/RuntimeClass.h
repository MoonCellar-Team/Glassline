#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class RuntimeClass : public midp::Class {
public:
    RuntimeClass();
    virtual ~RuntimeClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
