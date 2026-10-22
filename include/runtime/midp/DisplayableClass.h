#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class DisplayableClass : public midp::Class {
public:
    DisplayableClass();
    virtual ~DisplayableClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
