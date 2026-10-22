#include "game/UI/UnlockablePanel.h"

#include "game/UI/UnlockableItem.h"
#include "game/UI/UnlockableWindow.h"

static const int PANEL_WIDTH = 664;
static const int PANEL_HEIGHT = 320;
static const int NOTCH_WIDTH = 304;
static const int RENDER_EXTRA = 2;
static const float HALF_NOTCH = 152.0f;

namespace UI {

bool UnlockablePanel::pointerReleased(int x, int y, int id) {
    if (!m_dragging) {
        UnlockableItem* item = static_cast<UnlockableItem*>(m_selected);
        if (item->isUnlocked()) {
            m_window->displayUnlockable(item);
        }
    }
    NotchedSlider::pointerReleased(x, y, id);
    return true;
}

UnlockablePanel::UnlockablePanel(UnlockableWindow* window) : m_window(window) {
    setWidth(PANEL_WIDTH);
    setHeight(PANEL_HEIGHT);
    setNotchWidth(NOTCH_WIDTH);
    setRenderExtra(RENDER_EXTRA);
    m_halfNotchWidth = HALF_NOTCH;
}

UnlockablePanel::~UnlockablePanel() {}

}
