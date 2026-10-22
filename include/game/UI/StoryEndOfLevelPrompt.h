#pragma once

#include "game/UI/EndOfLevelPrompt.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class StoryEndOfLevelPrompt : public EndOfLevelPrompt {
public:
    StoryEndOfLevelPrompt();
    virtual ~StoryEndOfLevelPrompt();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual void UnHide();

    midp::ReferenceCountedPointer<midp::String> m_bags;
};

}
