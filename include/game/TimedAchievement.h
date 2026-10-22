#pragma once

#include "game/Achievement.h"

class TimedAchievement : public Achievement {
public:
    TimedAchievement(int idx, int nameId, int descriptionId, int seconds);
    virtual ~TimedAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    void runTimer(int time);
    void endTimer();

    float m_startTime;
    int m_seconds;
};
