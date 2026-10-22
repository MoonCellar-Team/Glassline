#pragma once

#include "runtime/midp/Runnable.h"

namespace midp {
class Class;
}

namespace util {

class TimerTask : public midp::Runnable {
public:
    TimerTask();
    virtual ~TimerTask();
    virtual void run();
    virtual midp::Class* getClass() const;
    bool cancel();
    bool wantsToCancel();

    int m_unk04;
    int m_unk08;
    bool m_unk0C;
    bool m_unk0D;
    bool m_cancelled;
};

}
