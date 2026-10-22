#pragma once

#include "game/Achievement.h"

class LevelAchievement : public Achievement {
public:
    LevelAchievement(int idx, int nameId, int descriptionId, int level);
    virtual ~LevelAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    int getLevel();

    int m_level;
};
