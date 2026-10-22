#include "game/UI/EndOfLevelPrompt.h"

#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
extern "C" midp::String* medecomp_player_level_title(void);
#endif

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;

static const int TEXT_RESTART = 0x8fd;
static const int TEXT_CONTINUE = 0x814;
static const int TEXT_NEXT = 0x815;
#ifdef MEDECOMP_PORT
static const int TEXT_SELECT = 0x9cf;
static int nextText() { return port::net::raceHostChooses() ? TEXT_SELECT : TEXT_NEXT; }
#endif
static const int TEXT_HEADING = 0x8b2;
static const int TEXT_BESIDE_NAME = 0x8b3;

static const int FONT_HEADING = 0x1c;
static const int FONT_NAME = 0x1d;
static const int ANCHOR_TOP_LEFT = 9;

static const int TITLE_POS = 0x28;
static const int NAME_EXTRA_Y = 0x2a;
static const int BUTTON_BOTTOM = 0x28;
static const int BUTTON_EDGE = 0x3c;
static const int CONTINUE_INSET = 0x4c;
static const int OFFSCREEN = -500;

namespace UI {

bool EndOfLevelPrompt::pointerPressed(int x, int y, int id) {
    if (m_restart.contains(x, y)) {
        m_restart.pointerPressed(x, y, id);
    } else if (m_restart.isPressed()) {
        m_restart.unpress();
    }
    if (m_next.contains(x, y)) {
        m_next.pointerPressed(x, y, id);
    } else if (m_next.isPressed()) {
        m_next.unpress();
    }
    if (m_continue.contains(x, y)) {
        m_continue.pointerPressed(x, y, id);
    } else if (m_continue.isPressed()) {
        m_continue.unpress();
    }
    return false;
}

bool EndOfLevelPrompt::pointerDragged(int x, int y, int id) {
    if (m_restart.isPressed() && !m_restart.contains(x, y)) {
        m_restart.unpress();
    }
    if (m_next.isPressed() && !m_next.contains(x, y)) {
        m_next.unpress();
    }
    if (m_continue.isPressed() && !m_continue.contains(x, y)) {
        m_continue.unpress();
    }
    return false;
}

void EndOfLevelPrompt::update(int dt) {
    if (!m_closing) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    SceneGame::GameState next = SceneGame::STATE_EXIT_FADE;
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_SPEEDRUN
        || AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_CHALLENGE) {
        next = SceneGame::STATE_UPLOAD_CONFIRM;
    }
    canvas->getSceneGame()->stateTransition(next);
    close(RESULT_NONE);
}

bool EndOfLevelPrompt::pointerReleased(int x, int y, int id) {
    AppEngine* canvas = AppEngine::getCanvas();
    if (m_restart.contains(x, y)) {
        m_restart.pointerReleased(x, y, id);
        canvas->getSceneGame()->setExitState(SceneGame::STATE_RESTART_LEVEL);
        m_closing = true;
    } else if (m_next.contains(x, y)) {
        m_next.pointerReleased(x, y, id);
        canvas->getSceneGame()->setExitState(SceneGame::STATE_NEXT_LEVEL);
        m_closing = true;
    } else if (m_continue.contains(x, y)) {
        m_continue.pointerReleased(x, y, id);
        canvas->getSceneGame()->setExitState(SceneGame::STATE_EXIT_LEVEL);
        m_closing = true;
    }
    return false;
}

void EndOfLevelPrompt::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    m_next.render(g, originY, originX);
    m_continue.render(g, originY, originX);
    m_restart.render(g, originY, originX);
    text->drawString(g, TEXT_HEADING, FONT_HEADING, TITLE_POS, TITLE_POS, ANCHOR_TOP_LEFT);
    int nameY = text->getLineHeight(FONT_HEADING) + NAME_EXTRA_Y;
    int nameId = level->getName();
#ifdef MEDECOMP_PORT
    midp::ReferenceCountedPointer<midp::String> name(medecomp_player_level_title());
    if (!name) {
        name = text->getString(nameId);
    }
    text->drawString(g, (midp::String*)name, FONT_NAME, TITLE_POS, nameY, ANCHOR_TOP_LEFT);
    midp::ReferenceCountedPointer<midp::String> space(new midp::String(" "));
    int nameWidth = text->getStringWidth((midp::String*)name, FONT_NAME);
#else
    text->drawString(g, nameId, FONT_NAME, TITLE_POS, nameY, ANCHOR_TOP_LEFT);
    midp::ReferenceCountedPointer<midp::String> space(new midp::String(" "));
    int nameWidth = text->getStringWidth(nameId, FONT_NAME);
#endif
    int gap = text->getStringWidth(space, FONT_NAME);
    text->drawString(g, TEXT_BESIDE_NAME, FONT_NAME, nameWidth + gap + TITLE_POS, nameY, ANCHOR_TOP_LEFT);
}

EndOfLevelPrompt::EndOfLevelPrompt()
    : Window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
      m_restart(TEXT_RESTART),
      m_continue(TEXT_CONTINUE),
#ifdef MEDECOMP_PORT
      m_next(nextText()),
#else
      m_next(TEXT_NEXT),
#endif
      m_story(false),
      m_lastStory(false),
      m_newlyCompleted(false),
      m_closing(false) {
    LevelData* levels = AppEngine::getLevelData();
    m_story = levels->getGameMode() == LevelData::GAME_MODE_STORY;
    m_lastStory = levels->isCurrentLevelLastStory();
    m_newlyCompleted = levels->isGameNewlyCompleted();

    int buttonY = m_height - m_restart.getHeight() - BUTTON_BOTTOM;
    int nextX = m_width - m_next.getWidth() - BUTTON_EDGE;
    int continueX = m_width - CONTINUE_INSET - (m_continue.getWidth() + m_next.getWidth());

    if (m_lastStory && !(m_story && m_newlyCompleted)
#ifdef MEDECOMP_PORT
        && !port::net::raceHostChooses()
#endif
    ) {
        m_next.setPosition(OFFSCREEN, OFFSCREEN);
    } else {
        m_next.setPosition(nextX, buttonY);
    }
    if (m_story) {
        m_restart.setPosition(OFFSCREEN, OFFSCREEN);
    } else {
        m_restart.setPosition(BUTTON_EDGE, buttonY);
    }
    if (m_newlyCompleted && m_story) {
        m_continue.setPosition(OFFSCREEN, OFFSCREEN);
    } else {
        m_continue.setPosition(continueX, buttonY);
    }
}

EndOfLevelPrompt::~EndOfLevelPrompt() {}

}
