#pragma once

#include "runtime/midp/Control.h"

namespace midp {

class VolumeControl : public Control {
public:
    VolumeControl();
    virtual ~VolumeControl();
    virtual void setVolume(int volume) = 0;
    virtual int getVolume() const = 0;
};

}
