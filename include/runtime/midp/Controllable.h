#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Controllable : public virtual Object {
public:
    Controllable();
    virtual ~Controllable();
};

}

bool checkAudioQueueError(long status);
