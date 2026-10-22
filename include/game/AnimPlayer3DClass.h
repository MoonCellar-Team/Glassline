#pragma once

#include "runtime/midp/Class.h"

class AnimPlayer3DClass : public midp::Class {
public:
    AnimPlayer3DClass();
    virtual ~AnimPlayer3DClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};
