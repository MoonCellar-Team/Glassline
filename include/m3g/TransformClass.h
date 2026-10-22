#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class TransformClass : public midp::Class {
public:
    TransformClass();
    virtual ~TransformClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
