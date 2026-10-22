#pragma once

#include "runtime/midp/Control.h"

namespace midp {

class PositionControl : public Control {
public:
    PositionControl();
    virtual ~PositionControl();
    virtual void setPosition(float x, float y, float z) = 0;
    virtual void setVelocity(float x, float y, float z) = 0;
    virtual void setReferenceDistance(float distance) = 0;
    virtual void setRolloffFactor(float factor) = 0;
    virtual void setSourceRelative(bool relative) = 0;
};

}
