#include "game/UI/HorizontalScrollBar.h"

#include "game/QuadManager.h"
#include "game/UI/Window.h"
#include "runtime/midp/Graphics.h"

static const int GROUP_SCROLL = 0x43;
static const int MESH_TRACK = 0x85;
static const int MESH_THUMB = 0x86;
static const int BAR_HEIGHT = 10;
static const int SCOPE_UI = 2;
static const int ANCHOR = QuadManager::ANCHOR_LEFT | QuadManager::ANCHOR_TOP;

namespace UI {

int HorizontalScrollBar::getOffset() const {
    return m_window->getClientOffsetX();
}

void HorizontalScrollBar::setOffset(int offset) const {
    m_window->setClientOffsetX(offset);
}

void HorizontalScrollBar::render(midp::Graphics* g, int originY, int originX) {
    m_quadManager->setGroupVisible(GROUP_SCROLL, true);
    m_quadManager->setGroupPosition(GROUP_SCROLL, (float)(m_x + originX), (float)(m_y + originY));
    m_quadManager->setMeshBounds(MESH_TRACK, 0.0f, 0.0f, (float)m_width, (float)m_height, ANCHOR);

    int offset = m_window->getClientOffsetX();
    float span = (float)m_window->getClientMaxX();
    float ratio = (float)(-offset) / span;
    float client = (float)m_window->getClientWidth();
    float track = (float)m_width;
    float thumb = (client / (client + span)) * track;
    m_quadManager->setMeshBounds(MESH_THUMB, (track - thumb) * ratio, 0.0f, thumb, (float)m_height, ANCHOR);
    m_quadManager->render(g, SCOPE_UI);
    m_quadManager->setGroupVisible(GROUP_SCROLL, false);
}

HorizontalScrollBar::~HorizontalScrollBar() {}

HorizontalScrollBar::HorizontalScrollBar(Window* window) : ScrollBar(window) {
    setHeight(BAR_HEIGHT);
}

}
