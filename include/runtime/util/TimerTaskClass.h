#pragma once

#include "runtime/midp/Class.h"

namespace util {

class TimerTaskClass : public midp::Class {
public:
    TimerTaskClass();
    virtual ~TimerTaskClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
