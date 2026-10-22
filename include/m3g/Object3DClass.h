#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class Object3DClass : public midp::Class {
public:
    Object3DClass();
    virtual ~Object3DClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
