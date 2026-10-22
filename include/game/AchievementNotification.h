#pragma once

#include <queue>

#include "game/UI/BorderedElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class Achievement;

namespace midp {
class String;
}

namespace UI {

class AchievementNotification : public BorderedElement {
public:
    AchievementNotification();
    virtual ~AchievementNotification();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);

    void addAchievement(Achievement* achievement);

    int m_state;
    int m_timer;
    float m_slide;
    float m_alpha;
    std::queue<Achievement*> m_queue;
    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_description;
};

}
