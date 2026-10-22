#include "game/UI/GameOptions.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"

static const int TEXT_TITLE = 0x822;
static const int TEXT_SUBTITLE = 0x81e;
static const int TEXT_TUTORIAL = 0x8ff;
static const int FONT_LABEL = 0xe;
static const int LABEL_ANCHOR = 0x14;
static const int FRAME_WIDTH = 0x284;
static const int FRAME_HEIGHT = 0xb9;
static const int CHECK_INSET_X = 0xfa;
static const int OFFSCREEN = -500;

namespace UI {

void GameOptions::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    m_tutorial.render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int x = m_tutorial.getX() - 0x10;
    int y = m_tutorial.getY() + (m_tutorial.getHeight() >> 1) + 2;
    text->drawString(g, TEXT_TUTORIAL, FONT_LABEL, x, y, LABEL_ANCHOR);
}

bool GameOptions::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    if (m_tutorial.contains(x, y)) {
        m_tutorial.pointerReleased(x, y, id);
        AppEngine::getCanvas()->setTutorialBoxes(m_tutorial.getChecked());
    } else if (m_upload.contains(x, y)) {
        m_upload.pointerReleased(x, y, id);
        AppEngine::getCanvas()->setUploadScores(m_upload.getChecked() == false);
    } else {
        return false;
    }
    AppEngine::getCanvas()->saveGameOptions();
    return true;
}

GameOptions::GameOptions()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE) {
    int frameX = (m_width - FRAME_WIDTH) >> 1;
    int frameY = (m_height - FRAME_HEIGHT) >> 1;
    m_background.setPosition(frameX, frameY);
    m_background.setDimensions(FRAME_WIDTH, FRAME_HEIGHT);
    int checkY = frameY + ((FRAME_HEIGHT - m_tutorial.getHeight()) >> 1);
    m_tutorial.setPosition(frameX + CHECK_INSET_X, checkY);
    m_tutorial.setChecked(AppEngine::getCanvas()->getTutorialBoxes());
    m_upload.setPosition(OFFSCREEN, OFFSCREEN);
}

GameOptions::~GameOptions() {}

}
