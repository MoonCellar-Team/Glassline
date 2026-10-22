#pragma once

#include "game/UI/NotchedSlider.h"

namespace UI {

class UnlockableItem;
class UnlockableWindow;

class UnlockablePanel : public NotchedSlider {
public:
    explicit UnlockablePanel(UnlockableWindow* window);
    virtual ~UnlockablePanel();

    virtual bool pointerReleased(int x, int y, int id);

    UnlockableWindow* m_window;
};

}
