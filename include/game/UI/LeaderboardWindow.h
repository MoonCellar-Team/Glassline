#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/LeaderboardListWindow.h"
#include "game/UI/LeftArrowButton.h"
#include "game/UI/MajorButton.h"
#include "game/UI/NotchedSlider.h"
#include "game/UI/RightArrowButton.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class LeaderboardWindow : public Window {
public:
    LeaderboardWindow();
    virtual ~LeaderboardWindow();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    int getItemCount() const;

    MajorButton m_back;
    MajorButton m_friendsButton;
    MajorButton m_globalButton;
    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_subtitle;
    BorderedElement m_frame;
    LeaderboardListWindow m_list;
    NotchedSlider m_slider;
    LeftArrowButton m_left;
    RightArrowButton m_right;
    bool m_friends;
    bool m_dragged;
    int m_selectedNotch;
};

}
