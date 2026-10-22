#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class RendererClass : public midp::Class {
public:
    RendererClass();
    virtual ~RendererClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
