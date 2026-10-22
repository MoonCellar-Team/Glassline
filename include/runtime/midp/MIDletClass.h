#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class MIDletClass : public midp::Class {
public:
    MIDletClass();
    virtual ~MIDletClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
