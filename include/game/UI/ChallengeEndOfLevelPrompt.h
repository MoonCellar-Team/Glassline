#pragma once

#include "game/UI/EndOfLevelPrompt.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class ChallengeEndOfLevelPrompt : public EndOfLevelPrompt {
public:
    ChallengeEndOfLevelPrompt();
    virtual ~ChallengeEndOfLevelPrompt();

    using EndOfLevelPrompt::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void UnHide();

    midp::ReferenceCountedPointer<midp::String> m_time;
    midp::ReferenceCountedPointer<midp::String> m_target;
    MajorButton m_challenge;
    bool m_hidden;
    bool m_won;
};

}
