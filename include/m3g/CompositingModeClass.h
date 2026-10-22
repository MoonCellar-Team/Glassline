#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class CompositingModeClass : public midp::Class {
public:
    CompositingModeClass();
    virtual ~CompositingModeClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
