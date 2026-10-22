#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ControlClass : public midp::Class {
public:
    ControlClass();
    virtual ~ControlClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
