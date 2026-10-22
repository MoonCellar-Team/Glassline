#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/ChallengeUserListWindow.h"
#include "game/UI/MajorButton.h"
#include "game/WrappedString.h"

namespace UI {

class ChallengeUserWindow : public Window {
public:
    ChallengeUserWindow();
    virtual ~ChallengeUserWindow();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    MajorButton m_backButton;
    MajorButton m_extraButton;
    BorderedElement m_frame;
    ChallengeUserListWindow m_users;
    WrappedString m_text;
    bool m_dragged;
};

}
