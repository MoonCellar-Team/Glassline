#pragma once

#include <string>

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"

namespace UI {

class PendingChallengeWindow : public Window {
public:
    PendingChallengeWindow(const std::string& challengeId, const std::string& name);
    virtual ~PendingChallengeWindow();

    const std::string& getChallengeID() const;

    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    std::string m_challengeId;
    BorderedElement m_frame;
    WrappedString m_name;
    WrappedString m_message;
    MajorButton m_accept;
    MajorButton m_decline;
    MajorButton m_later;
};

}
