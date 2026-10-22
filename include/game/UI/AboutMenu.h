#pragma once

#include "game/UI/AboutMenuPanel.h"
#include "game/UI/CheckBox.h"
#include "game/UI/LeftArrowButton.h"
#include "game/UI/RightArrowButton.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class AboutMenu : public TitledWindow {
public:
    AboutMenu();
    virtual ~AboutMenu();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    void setHidden(bool hidden);

    bool m_hidden;
    CheckBox m_share;
    AboutMenuPanel m_panel;
    LeftArrowButton m_prevButton;
    RightArrowButton m_nextButton;
};

}
