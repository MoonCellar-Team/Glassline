#pragma once

#include "game/UI/WindowElement.h"
#include "game/WrappedString.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Graphics;
class String;
}

namespace UI {

class AchievementsList;

class AchievementItem : public WindowElement {
public:
    AchievementItem(AchievementsList* list, int index);
    virtual ~AchievementItem();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int m_index;
    midp::ReferenceCountedPointer<midp::String> m_name;
    WrappedString m_description;
};

}
