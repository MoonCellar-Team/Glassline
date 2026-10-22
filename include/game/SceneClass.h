#pragma once

#include "runtime/midp/Class.h"

class SceneClass : public midp::Class {
public:
    SceneClass();
    virtual ~SceneClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};
