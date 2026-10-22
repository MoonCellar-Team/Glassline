#include "game/UI/EndOfGameResultsWindow.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"

static const int TEXT_BUTTON = 0x904;
static const int TEXT_TITLE = 0x91c;
static const int TEXT_CHAPTERS = 0x91b;
static const int TEXT_BAGS = 0x842;
static const int TEXT_TOTAL = 0x91d;
static const int TEXT_OF = 0x800;

static const int BUTTON_MARGIN = 50;
static const int FONT_TITLE = 28;
static const int FONT_HEADER = 17;
static const int FONT_ROW = 2;
static const int ANCHOR_TITLE = 66;
static const int ANCHOR_NAME = 65;

static const int TITLE_Y = 110;
static const int HEADER_Y = 166;
static const int CHAPTER_X = 110;
static const int BAGS_X = 0x2a3;
static const int TOTAL_X = 0x36c;
static const int FIRST_ROW_Y = 250;
static const int ROW_STEP = 27;
static const int SKIPPED_LEVELS = 4;

namespace UI {

bool EndOfGameResultsWindow::pointerPressed(int x, int y, int id) {
    if (!m_button.contains(x, y)) {
        return false;
    }
    if (m_button.getStringId() == -1) {
        return false;
    }
    m_button.pointerPressed(x, y, id);
    return true;
}

bool EndOfGameResultsWindow::pointerDragged(int x, int y, int id) {
    if (m_button.isPressed() && !m_button.contains(x, y)) {
        m_button.unpress();
    }
    return false;
}

void EndOfGameResultsWindow::render(midp::Graphics* g, int originY, int originX) {
    AppEngine* canvas = AppEngine::getCanvas();
    int screenWidth = canvas->getWidth();
    LevelData* levels = AppEngine::getLevelData();
    TextManager* text = canvas->getTextManager();
    text->drawString(g, m_title, FONT_TITLE, screenWidth >> 1, TITLE_Y, ANCHOR_TITLE);
    text->drawString(g, m_chapterHeader, FONT_HEADER, CHAPTER_X, HEADER_Y, ANCHOR_NAME);
    text->drawString(g, m_bagsHeader, FONT_HEADER, BAGS_X, HEADER_Y, ANCHOR_TITLE);

    int foundSum = 0;
    int totalSum = 0;
    int count = levels->getLevelNum();
    if (count != SKIPPED_LEVELS) {
        int rows = count - SKIPPED_LEVELS;
        int rowY = FIRST_ROW_Y;
        for (int i = 0; i != rows; ++i) {
            const Level* level = levels->getLevel(i);
            text->drawString(g, level->getName(), FONT_ROW, CHAPTER_X, rowY, ANCHOR_NAME);
            int found = level->getNumBagsFound();
            int total = level->getNumTotalBags();
            foundSum += found;
            totalSum += total;
            canvas->drawOfStatString(g, FONT_ROW, TEXT_OF, found, total, BAGS_X, rowY, ANCHOR_TITLE);
            rowY += ROW_STEP;
        }
    }
    text->drawString(g, TEXT_TOTAL, FONT_ROW, TOTAL_X, HEADER_Y, ANCHOR_TITLE);
    canvas->drawOfStatString(g, FONT_TITLE, TEXT_OF, foundSum, totalSum, TOTAL_X, FIRST_ROW_Y, ANCHOR_TITLE);
    m_button.render(g, 0, 0);
}

bool EndOfGameResultsWindow::pointerReleased(int x, int y, int id) {
    if (!m_button.contains(x, y)) {
        return false;
    }
    if (m_button.getStringId() == -1) {
        return false;
    }
    m_button.pointerReleased(x, y, id);
    close(RESULT_OK);
    return true;
}

EndOfGameResultsWindow::EndOfGameResultsWindow() : m_button(TEXT_BUTTON) {
    AppEngine* canvas = AppEngine::getCanvas();
    int screenWidth = canvas->getWidth();
    int screenHeight = canvas->getHeight();
    int x = screenWidth - BUTTON_MARGIN - m_button.getWidth();
    int y = screenHeight - BUTTON_MARGIN - m_button.getHeight();
    m_button.setPosition(x, y);

    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_title = text->getString(TEXT_TITLE)->toUpperCase();
    m_chapterHeader = text->getString(TEXT_CHAPTERS)->toUpperCase();
    m_bagsHeader = text->getString(TEXT_BAGS)->toUpperCase();
}

EndOfGameResultsWindow::~EndOfGameResultsWindow() {}

}
