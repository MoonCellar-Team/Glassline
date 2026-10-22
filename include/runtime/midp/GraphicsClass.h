#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class GraphicsClass : public midp::Class {
public:
    GraphicsClass();
    virtual ~GraphicsClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
