#pragma once

#include "runtime/midp/Control.h"

namespace midp {

class PitchControl : public Control {
public:
    PitchControl();
    virtual ~PitchControl();
    virtual void setPitchFactor(float factor) = 0;
    virtual float getPitchFactor() const = 0;
};

}
