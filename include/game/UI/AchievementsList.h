#pragma once

#include "game/UI/Window.h"

class AchievementData;

namespace midp {
class Graphics;
}

namespace UI {

class AchievementsList : public Window {
public:
    AchievementsList(int x, int y, int width, int height);
    virtual ~AchievementsList();

    virtual void render(midp::Graphics* g, int originY, int originX);

    AchievementData* getAchievementData() const;

    AchievementData* m_achievementData;
};

}
