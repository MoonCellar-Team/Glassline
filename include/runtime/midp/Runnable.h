#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Runnable : public virtual Object {
public:
    Runnable();
    virtual ~Runnable();
    virtual void run();
};

}
