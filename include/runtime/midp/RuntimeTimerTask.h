#pragma once

#include "runtime/util/TimerTask.h"

namespace midp {

class RuntimeTimerTask : public util::TimerTask {
public:
    RuntimeTimerTask();
    virtual ~RuntimeTimerTask();
    virtual void run();
};

}
