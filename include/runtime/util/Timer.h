#pragma once

#include "runtime/midp/Object.h"

namespace midp {
class Class;
}

namespace util {

class TimerTask;

class Timer : public midp::Object {
public:
    Timer();
    virtual ~Timer();
    virtual midp::Class* getClass() const;
    void setTask(TimerTask* task);
    TimerTask* getTask() const;
    void schedule(TimerTask* task, long delay, long period);
    void cancel();

    TimerTask* m_task;
    long m_delay;
    long m_period;
    bool m_unk14;
};

}
