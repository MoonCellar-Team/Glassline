#include "game/UI/VerticalScrollBar.h"

#include "game/QuadManager.h"
#include "game/UI/Window.h"
#include "runtime/midp/Graphics.h"

static const int GROUP_SCROLL = 0x43;
static const int MESH_TRACK = 0x85;
static const int MESH_THUMB = 0x86;
static const int BAR_WIDTH = 16;
static const int SCOPE_UI = 2;
static const int ANCHOR = QuadManager::ANCHOR_LEFT | QuadManager::ANCHOR_TOP;

namespace UI {

int VerticalScrollBar::getOffset() const {
    return m_window->getClientOffsetY();
}

void VerticalScrollBar::setOffset(int offset) const {
    m_window->setClientOffsetY(offset);
}

void VerticalScrollBar::render(midp::Graphics* g, int originY, int originX) {
    m_quadManager->setGroupVisible(GROUP_SCROLL, true);
    m_quadManager->setGroupPosition(GROUP_SCROLL, (float)(m_x + originX), (float)(m_y + originY));
    m_quadManager->setMeshBounds(MESH_TRACK, 0.0f, 0.0f, (float)m_width, (float)m_height, ANCHOR);

    int offset = m_window->getClientOffsetY();
    float span = (float)m_window->getClientMaxY();
    float ratio = (float)(-offset) / span;
    float client = (float)m_window->getClientHeight();
    float track = (float)m_height;
    float thumb = (client / (client + span)) * track;
    m_quadManager->setMeshBounds(MESH_THUMB, 0.0f, (track - thumb) * ratio, (float)m_width, thumb, ANCHOR);
    m_quadManager->render(g, SCOPE_UI);
    m_quadManager->setGroupVisible(GROUP_SCROLL, false);
}

VerticalScrollBar::~VerticalScrollBar() {}

VerticalScrollBar::VerticalScrollBar(Window* window) : ScrollBar(window) {
    setWidth(BAR_WIDTH);
}

}
