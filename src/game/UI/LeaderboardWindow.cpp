#include "game/UI/LeaderboardWindow.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/TextManager.h"
#include "game/UI/ChapterSelectItemSpeedrun.h"
#include "runtime/midp/String.h"

static const int TEXT_BACK = 0x82f;
static const int TEXT_FRIENDS = 0x92e;
static const int TEXT_GLOBAL = 0x92f;
static const int TEXT_TITLE = 0x95b;
static const int TEXT_SUBTITLE = 0x823;
static const int TEXT_RANK = 0x92b;
static const int TEXT_NAME = 0x92c;
static const int TEXT_TIME = 0x830;

static const int FONT_TITLE = 0x10;
static const int FONT_SUBTITLE = 0x11;
static const int FONT_COLUMN = 0x13;
static const int ANCHOR_TOP_LEFT = 9;
static const int ANCHOR_SUBTITLE = 0x21;

static const int TITLE_X = 0x9b;
static const int TITLE_Y = 0x50;
static const int SUBTITLE_Y = 0x49;
static const int COLUMN_DY = 0x18;
static const int COLUMN_RANK_DX = 0x14;
static const int COLUMN_NAME_DX = 100;
static const int COLUMN_TIME_DX = 0x2da;

static const int FRAME_X = 0x10;
static const int FRAME_Y = 0xdc;
static const int FRAME_WIDTH_CUT = 0x20;
static const int FRAME_HEIGHT = 0x19c;
static const int LIST_X = 0x26;
static const int LIST_Y = 0x118;
static const int LIST_WIDTH_CUT = 0x38;
static const int LIST_HEIGHT = 0x15e;

static const int BACK_X_CUT = 0x45;
static const int BACK_Y_CUT = 0x24;
static const int OFFSCREEN = -3000;
static const int NOTCH_WIDTH_CUT = 0x136;
static const int SLIDER_HEIGHT = 0x9b;
static const int SLIDER_Y = 0x40;
static const int ARROW_GAP = 0x40;
static const int RENDER_EXTRA = 1;

static const int LEVEL_TYPE_SKIP = 2;
static const UI::WindowResult RESULT_BACK = (UI::WindowResult)3;

namespace UI {

bool LeaderboardWindow::pointerDragged(int x, int y, int id) {
    m_dragged = true;
    if (m_slider.contains(x, y)) {
        m_slider.pointerDragged(m_slider.toRelativeX(x), y, id);
    } else if (m_slider.isDragging()) {
        m_slider.pointerReleased(x, y, id);
    }
    if (!m_left.contains(x, y) && m_right.isPressed()) {
        m_left.unpress();
    } else if (!m_right.contains(x, y) && m_right.isPressed()) {
        m_right.unpress();
    } else if (!m_back.contains(x, y) && m_back.isPressed()) {
        m_back.unpress();
    } else if (!m_globalButton.contains(x, y) && !m_globalButton.isPressed()) {
        m_globalButton.unpress();
    } else if (!m_friendsButton.contains(x, y) && !m_friendsButton.isPressed()) {
        m_friendsButton.unpress();
    }
    if (m_list.contains(x, y)) {
        m_list.pointerDragged(m_list.toRelativeX(x), m_list.toRelativeY(y), id);
    }
    return true;
}

bool LeaderboardWindow::pointerPressed(int x, int y, int id) {
    if (m_slider.contains(x, y)) {
        m_slider.pointerPressed(m_slider.toRelativeX(x), y, id);
    } else if (m_left.contains(x, y)) {
        m_left.pointerPressed(x, y, id);
    } else if (m_right.contains(x, y)) {
        m_right.pointerPressed(x, y, id);
    } else if (m_back.contains(x, y)) {
        m_back.pointerPressed(x, y, id);
    } else if (m_globalButton.contains(x, y)) {
        m_globalButton.pointerPressed(x, y, id);
    } else if (m_friendsButton.contains(x, y)) {
        m_friendsButton.pointerPressed(x, y, id);
    }
    if (m_list.contains(x, y)) {
        m_list.pointerPressed(m_list.toRelativeX(x), m_list.toRelativeY(y), id);
    }
    return true;
}

LeaderboardWindow::~LeaderboardWindow() {}

void LeaderboardWindow::update(int dt) {
    m_slider.update(dt);
    m_right.setEnabled(m_slider.canNext());
    m_left.setEnabled(m_slider.canPrev());
    m_list.update(dt);
    int notch = m_slider.getSelectedNotch();
    if (m_selectedNotch != notch) {
        m_selectedNotch = notch;
        m_list.SetListIdx(notch, m_friends);
    }
}

void LeaderboardWindow::render(midp::Graphics* g, int originY, int originX) {
    m_back.render(g, originY, originX);
    m_friendsButton.render(g, originY, originX);
    m_globalButton.render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, m_title, FONT_TITLE, TITLE_X, TITLE_Y, ANCHOR_TOP_LEFT);
    text->drawString(g, m_subtitle, FONT_SUBTITLE, TITLE_X, SUBTITLE_Y, ANCHOR_SUBTITLE);
    m_slider.render(g, originY, originX);
    m_left.render(g, originY, originX);
    m_right.render(g, originY, originX);
    m_frame.render(g, originY, originX);
    if (m_slider.isIdle()) {
        m_list.render(g, originY, originX);
    }
    int y = m_frame.getY() + COLUMN_DY;
    int x = m_list.getX();
    text->drawString(g, TEXT_RANK, FONT_COLUMN, x + COLUMN_RANK_DX, y, ANCHOR_TOP_LEFT);
    text->drawString(g, TEXT_NAME, FONT_COLUMN, x + COLUMN_NAME_DX, y, ANCHOR_TOP_LEFT);
    text->drawString(g, TEXT_TIME, FONT_COLUMN, x + COLUMN_TIME_DX, y, ANCHOR_TOP_LEFT);
}

bool LeaderboardWindow::pointerReleased(int x, int y, int id) {
    if (m_slider.contains(x, y)) {
        m_slider.pointerReleased(x, y, id);
    }
    if (m_left.contains(x, y)) {
        m_slider.prev();
        m_left.pointerReleased(x, y, id);
    } else if (m_left.isPressed()) {
        m_left.unpress();
    }
    if (m_right.contains(x, y)) {
        m_slider.next();
        m_right.pointerReleased(x, y, id);
    } else if (m_right.isPressed()) {
        m_right.unpress();
    }
    if (m_back.contains(x, y)) {
        m_back.pointerReleased(x, y, id);
        close(RESULT_BACK);
    } else if (m_back.isPressed()) {
        m_back.unpress();
    }
    if (m_globalButton.contains(x, y)) {
        m_globalButton.pointerReleased(x, y, id);
        m_friends = false;
        m_list.SetListIdx(0, false);
        m_slider.setTargetNotch(0);
    } else if (m_globalButton.isPressed()) {
        m_globalButton.unpress();
    }
    if (m_friendsButton.contains(x, y)) {
        m_friendsButton.pointerReleased(x, y, id);
        m_friends = true;
        m_list.SetListIdx(0, true);
        m_slider.setTargetNotch(0);
    } else if (m_friendsButton.isPressed()) {
        m_friendsButton.unpress();
    }
    if (m_list.contains(x, y) && !m_dragged) {
        m_list.pointerReleased(m_list.toRelativeX(x), m_list.toRelativeY(y), id);
    }
    m_dragged = false;
    return true;
}

int LeaderboardWindow::getItemCount() const {
    LevelData* levels = AppEngine::getLevelData();
    int unlocked = levels->getNumUnlockedLevels();
    int levelNum = levels->getLevelNum();
    int count = 0;
    if (levelNum >= 1) {
        for (int i = 0; i < levelNum; ++i) {
            const Level* level = levels->getLevel(i);
            if (i < unlocked || level->isLevelComplete() || i == 0) {
                if (level->getType() != LEVEL_TYPE_SKIP) {
                    ++count;
                }
            }
        }
    }
    return count;
}

LeaderboardWindow::LeaderboardWindow()
    : Window(),
      m_back(TEXT_BACK),
      m_friendsButton(TEXT_FRIENDS),
      m_globalButton(TEXT_GLOBAL),
      m_title(NULL),
      m_subtitle(NULL),
      m_frame(FRAME_X, FRAME_Y, m_width - FRAME_WIDTH_CUT, FRAME_HEIGHT),
      m_list(*this, LIST_X, LIST_Y, m_width - LIST_WIDTH_CUT, LIST_HEIGHT),
      m_slider(),
      m_left(),
      m_right(),
      m_friends(false),
      m_dragged(false),
      m_selectedNotch(0) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_title = text->getString(TEXT_TITLE)->toUpperCase();
    m_subtitle = text->getString(TEXT_SUBTITLE)->toUpperCase();

    int backX = (m_width - BACK_X_CUT) - m_back.getWidth();
    int backY = (m_height - BACK_Y_CUT) - m_back.getHeight();
    m_back.setPosition(backX, backY);
    m_friendsButton.setPosition(OFFSCREEN, OFFSCREEN);
    m_globalButton.setPosition(OFFSCREEN, OFFSCREEN);

    int notchWidth = m_width - NOTCH_WIDTH_CUT;
    m_slider.setLooped(false);
    m_slider.setNotchWidth(notchWidth);
    m_slider.setRenderExtra(RENDER_EXTRA);
    m_slider.setDimensions(notchWidth, SLIDER_HEIGHT);
    m_slider.setPosition((m_width - m_slider.getWidth()) >> 1, SLIDER_Y);

    int arrowY = m_slider.getY() + ((m_slider.getHeight() - m_left.getHeight()) >> 1);
    m_left.setPosition(m_slider.getX() - ARROW_GAP - m_left.getWidth(), arrowY);
    m_right.setPosition(m_slider.getX() + m_slider.getWidth() + ARROW_GAP, arrowY);
    m_right.setEnabled(m_slider.canNext());
    m_left.setEnabled(m_slider.canPrev());

    LevelData* levels = AppEngine::getLevelData();
    int unlocked = levels->getNumUnlockedLevels();
    int levelNum = levels->getLevelNum();
    if (levelNum >= 1) {
        for (int i = 0; i < levelNum; ++i) {
            const Level* level = levels->getLevel(i);
            bool locked = false;
            if (i >= unlocked) {
                locked = !level->isLevelComplete();
            }
            int type = level->getType();
            if ((i == 0 || !locked) && type != LEVEL_TYPE_SKIP) {
                m_slider.addItem(new ChapterSelectItemSpeedrun(*level));
            }
        }
    }
    m_selectedNotch = 0;
}

}
