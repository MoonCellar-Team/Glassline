#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class InputStreamClass : public midp::Class {
public:
    InputStreamClass();
    virtual ~InputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
