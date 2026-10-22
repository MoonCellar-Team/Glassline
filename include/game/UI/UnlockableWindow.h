#pragma once

#include "game/UI/LeftArrowButton.h"
#include "game/UI/RightArrowButton.h"
#include "game/UI/TitledWindow.h"
#include "game/UI/UnlockablePanel.h"

namespace UI {

class UnlockableItem;

class UnlockableWindow : public TitledWindow {
public:
    UnlockableWindow();
    virtual ~UnlockableWindow();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    void displayUnlockable(UnlockableItem* item);

    int m_phase;
    int m_timer;
    UnlockableItem* m_item;
    UnlockablePanel m_panel;
    MajorButton m_save;
    bool m_saved;
    LeftArrowButton m_left;
    RightArrowButton m_right;
};

}
