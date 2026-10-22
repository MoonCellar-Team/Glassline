#pragma once

#include "runtime/midp/Class.h"

namespace util {

class TimerClass : public midp::Class {
public:
    TimerClass();
    virtual ~TimerClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
