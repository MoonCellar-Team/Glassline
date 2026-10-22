#include "game/UI/WindowWithBackground.h"

#include "game/QuadManager.h"

static const int QUAD_GROUP_FRAME = 71;
static const int ANIM_FRAME_WIDTH = 18;
static const int ANIM_FRAME_HEIGHT = 19;

namespace UI {

void WindowWithBackground::render(midp::Graphics* g, int originY, int originX) {
    m_quadManager->setGroupVisible(QUAD_GROUP_FRAME, true);
    m_quadManager->setGroupPosition(QUAD_GROUP_FRAME, (float)(m_x + originX), (float)(m_y + originY));
    m_quadManager->setAnimTime(ANIM_FRAME_WIDTH, m_width);
    m_quadManager->setAnimTime(ANIM_FRAME_HEIGHT, m_height);
    m_quadManager->render(g, 2);
    m_quadManager->setGroupVisible(QUAD_GROUP_FRAME, false);
    Window::render(g, originY, originX);
}

WindowWithBackground::WindowWithBackground(int x, int y, int width, int height) : Window(x, y, width, height) {}

WindowWithBackground::~WindowWithBackground() {}

}
