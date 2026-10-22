#pragma once

#include "game/Achievement.h"

class CountAchievement : public Achievement {
public:
    CountAchievement(int idx, int nameId, int descriptionId, int count);
    virtual ~CountAchievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    int getCount() const;

    int m_count;
};
