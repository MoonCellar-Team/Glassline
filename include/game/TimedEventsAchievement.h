#pragma once

#include "game/Achievement.h"
#include "runtime/midp/array.h"

class TimedEventsAchievement : public Achievement {
public:
    TimedEventsAchievement(int idx, int nameId, int descriptionId, int events, int seconds);
    virtual ~TimedEventsAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    void levelStarted();
    void eventHappended(int time);

    int m_seconds;
    midp::array<int> m_eventTimes;
};
