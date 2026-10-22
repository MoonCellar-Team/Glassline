#pragma once

#include "game/LevelAchievement.h"

class BagsPerLevelAchievement : public LevelAchievement {
public:
    BagsPerLevelAchievement(int idx, int nameId, int descriptionId, int level, int bags);
    virtual ~BagsPerLevelAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    int getBags() const;

    int m_bags;
};
