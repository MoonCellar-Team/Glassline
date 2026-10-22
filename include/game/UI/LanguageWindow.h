#pragma once

#include "game/UI/LanguagePanel.h"
#include "game/UI/LeftArrowButton.h"
#include "game/UI/RightArrowButton.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class LanguageWindow : public TitledWindow {
public:
    LanguageWindow();
    virtual ~LanguageWindow();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    int m_language;
    LanguagePanel m_languages;
    LeftArrowButton m_left;
    RightArrowButton m_right;
};

}
