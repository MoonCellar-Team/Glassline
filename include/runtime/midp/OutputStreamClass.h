#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class OutputStreamClass : public midp::Class {
public:
    OutputStreamClass();
    virtual ~OutputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
