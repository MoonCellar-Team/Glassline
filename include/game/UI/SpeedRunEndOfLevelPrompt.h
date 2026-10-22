#pragma once

#include "game/UI/EndOfLevelPrompt.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class SpeedRunEndOfLevelPrompt : public EndOfLevelPrompt {
public:
    SpeedRunEndOfLevelPrompt();
    virtual ~SpeedRunEndOfLevelPrompt();

    using EndOfLevelPrompt::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void UnHide();

    midp::ReferenceCountedPointer<midp::String> m_time;
    midp::ReferenceCountedPointer<midp::String> m_best;
    MajorButton m_challenge;
    bool m_hidden;
};

}
