#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class VertexBufferClass : public midp::Class {
public:
    VertexBufferClass();
    virtual ~VertexBufferClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
