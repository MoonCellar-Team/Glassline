#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class DisplayClass : public midp::Class {
public:
    DisplayClass();
    virtual ~DisplayClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
