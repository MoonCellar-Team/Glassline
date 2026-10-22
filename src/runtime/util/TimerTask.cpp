#include "runtime/util/TimerTask.h"

#include "runtime/util/TimerTaskClass.h"

namespace util {

midp::Class* TimerTask::getClass() const {
    return new TimerTaskClass();
}

bool TimerTask::cancel() {
    if (m_cancelled)
        return false;
    m_cancelled = true;
    if (!m_unk0C && !m_unk0D)
        return true;
    return m_unk0C != 0;
}

void TimerTask::run() {
    m_unk0D = true;
}

bool TimerTask::wantsToCancel() {
    return m_cancelled;
}

TimerTask::~TimerTask() {}

TimerTask::TimerTask() : m_unk04(0), m_unk08(0), m_unk0C(false), m_unk0D(false), m_cancelled(false) {}

}
