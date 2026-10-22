#include "game/UI/AchievementWindow.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"

static const int TEXT_TITLE = 0x825;
static const int TEXT_SUBTITLE = 0x823;
static const int TEXT_LEFT = 0x86e;
static const int TEXT_RIGHT = 0x86f;
static const int FONT_COLUMN = 0x13;
static const int ANCHOR_LEFT = 9;
static const int ANCHOR_RIGHT = 0xc;
static const int COLUMN_LEFT_X = 0x150;
static const int COLUMN_RIGHT_X = 0x370;
static const int COLUMN_Y = 0x14;

static const int LIST_X = 0x60;
static const int LIST_Y_BIAS = 0x32;
static const int LIST_WIDTH_CUT = 0xc0;
static const int LIST_HEIGHT = 0x18e;
static const int FRAME_X = 0x50;
static const int FRAME_HEIGHT = 0x1e0;
static const int FRAME_WIDTH_CUT = 0xa0;

namespace UI {

static bool forwardToList(AchievementsList& list, int x, int y, int id,
                          bool (Window::*method)(int, int, int)) {
    if (!list.contains(x, y)) {
        return false;
    }
    (list.*method)(x - list.getX(), y - list.getY(), id);
    return true;
}

bool AchievementWindow::pointerDragged(int x, int y, int id) {
    TitledWindow::pointerDragged(x, y, id);
    return forwardToList(m_list, x, y, id, &Window::pointerDragged);
}

bool AchievementWindow::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
    return forwardToList(m_list, x, y, id, &Window::pointerPressed);
}

void AchievementWindow::update(int dt) {
    TitledWindow::update(dt);
    m_list.update(dt);
}

void AchievementWindow::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int y = m_background.getY() + COLUMN_Y;
    text->drawString(g, TEXT_LEFT, FONT_COLUMN, COLUMN_LEFT_X, y, ANCHOR_LEFT);
    text->drawString(g, TEXT_RIGHT, FONT_COLUMN, COLUMN_RIGHT_X, y, ANCHOR_RIGHT);
    m_list.render(g, originY, originX);
}

bool AchievementWindow::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    return forwardToList(m_list, x, y, id, &Window::pointerReleased);
}

AchievementWindow::AchievementWindow()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_list(LIST_X, ((m_height - FRAME_HEIGHT) >> 1) + LIST_Y_BIAS, m_width - LIST_WIDTH_CUT,
             LIST_HEIGHT) {
    m_background.setPosition(FRAME_X, (m_height - FRAME_HEIGHT) >> 1);
    m_background.setDimensions(m_width - FRAME_WIDTH_CUT, FRAME_HEIGHT);
}

AchievementWindow::~AchievementWindow() {}

}
