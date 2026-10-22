#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class CanvasClass : public midp::Class {
public:
    CanvasClass();
    virtual ~CanvasClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
