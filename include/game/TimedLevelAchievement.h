#pragma once

#include "game/LevelAchievement.h"

class TimedLevelAchievement : public LevelAchievement {
public:
    TimedLevelAchievement(int idx, int nameId, int descriptionId, int level, int seconds);
    virtual ~TimedLevelAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    int getTime() const;

    int m_seconds;
};
