#include "game/UI/SpeedRunEndOfLevelPrompt.h"

#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "game/QuadManager.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
extern "C" int medecomp_player_level_playing(void);
extern "C" int medecomp_player_level_best(void);
extern "C" int medecomp_challenge_allowed(void);
#endif

static const int TEXT_CHALLENGE = 0x92d;
static const int TEXT_TIME = 0x90d;
static const int TEXT_BEST = 0x90e;

static const int FONT_TIME = 0x20;
static const int FONT_TIME_LABEL = 0x1d;
static const int FONT_BEST = 0x21;
static const int FONT_BEST_LABEL = 0x22;

static const int ANCHOR_BASELINE_LEFT = 0x41;
static const int ANCHOR_BASELINE_RIGHT = 0x44;

static const int TIME_X_INSET = 0x92;
static const int LABEL_X_INSET = 0x9a;
static const int ROW_Y = 0x24;
static const int ROW_EXTRA_Y = 0x26;
static const int TIME_DECIMALS = 2;

static const int GROUP_STARS = 0x49;
static const int MESH_STAR_1 = 0xa2;
static const int MESH_STAR_2 = 0xa3;
static const int MESH_STAR_3 = 0xa4;

static const int CHALLENGE_OFFSCREEN = -3000;
#ifdef MEDECOMP_PORT
static const int HEADLINE_ABOVE_BUTTONS = 14;
#endif

namespace UI {

static const WindowResult RESULT_CHALLENGE = static_cast<WindowResult>(3);

bool SpeedRunEndOfLevelPrompt::pointerPressed(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    EndOfLevelPrompt::pointerPressed(x, y, id);
    if (m_challenge.contains(x, y)) {
        m_challenge.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

bool SpeedRunEndOfLevelPrompt::pointerDragged(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    EndOfLevelPrompt::pointerDragged(x, y, id);
    if (!m_challenge.contains(x, y) && m_challenge.isPressed()) {
        m_challenge.unpress();
    }
    return false;
}

SpeedRunEndOfLevelPrompt::~SpeedRunEndOfLevelPrompt() {
    AppEngine::getCanvas()->getQuadManager().setGroupVisible(GROUP_STARS, false);
}

void SpeedRunEndOfLevelPrompt::render(midp::Graphics* g, int originY, int originX) {
    if (m_hidden) {
        return;
    }
    EndOfLevelPrompt::render(g, originY, originX);
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    int timeX = m_width - TIME_X_INSET;
    int labelX = m_width - LABEL_X_INSET;
    text->drawString(g, m_time, FONT_TIME, timeX, ROW_Y, ANCHOR_BASELINE_LEFT);
    text->drawString(g, TEXT_TIME, FONT_TIME_LABEL, labelX, ROW_Y, ANCHOR_BASELINE_RIGHT);
    int rowY = text->getLineHeight(FONT_BEST) + ROW_EXTRA_Y;
    text->drawString(g, m_best, FONT_BEST, timeX, rowY, ANCHOR_BASELINE_LEFT);
    text->drawString(g, TEXT_BEST, FONT_BEST_LABEL, labelX, rowY, ANCHOR_BASELINE_RIGHT);
#ifdef MEDECOMP_PORT
    port::net::renderRaceResult(g, labelX, timeX, rowY + rowY - ROW_Y, m_width / 2,
                                m_continue.getY() - HEADLINE_ABOVE_BUTTONS);
#endif
    canvas->getQuadManager().render(g);
    m_challenge.render(g, originY, originX);
}

bool SpeedRunEndOfLevelPrompt::pointerReleased(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    EndOfLevelPrompt::pointerReleased(x, y, id);
    if (m_challenge.contains(x, y)) {
        m_challenge.pointerReleased(x, y, id);
        AppEngine::getCanvas()->getSceneGame()->stateTransition(SceneGame::STATE_CHALLENGE_USER);
        close(RESULT_CHALLENGE);
        return true;
    }
    return false;
}

SpeedRunEndOfLevelPrompt::SpeedRunEndOfLevelPrompt()
    : m_time(NULL),
      m_best(NULL),
      m_challenge(TEXT_CHALLENGE),
      m_hidden(false) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TextManager* text = canvas->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();

    int raceTime = canvas->getSceneGame()->getRaceTime();
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendMillisTimeToBuffer(buffer, raceTime, TIME_DECIMALS);
    m_time = buffer->toString();

    int bestTime = level->getBestSpeedRunTimeMillis();
#ifdef MEDECOMP_PORT
    bool own = medecomp_player_level_playing() != 0;
    if (own) {
        bestTime = medecomp_player_level_best();
        if (bestTime < 0) {
            bestTime = raceTime;
        }
    }
#endif
    buffer = text->clearStringBuffer();
    text->appendMillisTimeToBuffer(buffer, bestTime, TIME_DECIMALS);
    m_best = buffer->toString();

    int stars = level->getNumStarsAchieved();
#ifdef MEDECOMP_PORT
    if (own) {
        stars = 0;
    }
#endif
    quads.setGroupVisible(GROUP_STARS, true);
    quads.setMeshVisible(MESH_STAR_1, stars > 0);
    quads.setMeshVisible(MESH_STAR_2, stars > 1);
    quads.setMeshVisible(MESH_STAR_3, stars > 2);

    int index = AppEngine::getLevelData()->getCurrentLevelIndex();
    int unlocked = AppEngine::getLevelData()->getNumUnlockedLevels();
    int last = AppEngine::getLevelData()->getLevelNum() - 1;
    if (!(last > index && (unlocked - 1) > index)
#ifdef MEDECOMP_PORT
        || own
#endif
    ) {
#ifdef MEDECOMP_PORT
        if (!port::net::raceHostChooses())
#endif
        m_next.setPosition(-m_next.getWidth(), -m_next.getHeight());
    }
    m_challenge.setPosition(CHALLENGE_OFFSCREEN, CHALLENGE_OFFSCREEN);
#ifdef MEDECOMP_PORT
    if (canvas->IsMayhemSessionValid() && medecomp_challenge_allowed()) {
        m_challenge.setPosition((m_width - m_challenge.getWidth()) / 2, m_restart.getY());
    }
#endif
#ifdef MEDECOMP_PORT
    if (port::net::raceTogether()) {
        m_challenge.setPosition(CHALLENGE_OFFSCREEN, CHALLENGE_OFFSCREEN);
        if (!port::net::raceIsHost()) {
            m_restart.setPosition(-m_restart.getWidth(), -m_restart.getHeight());
            m_next.setPosition(-m_next.getWidth(), -m_next.getHeight());
        }
    }
#endif
}

void SpeedRunEndOfLevelPrompt::UnHide() {
    m_hidden = false;
}

}
