#include "game/UI/ChallengeEndOfLevelPrompt.h"

#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#ifdef MEDECOMP_PORT
extern "C" int medecomp_challenge_allowed(void);
extern "C" midp::String* medecomp_player_level_title(void);
#endif

static const int TEXT_CHALLENGE = 0x92d;
static const int TEXT_TIME = 0x90d;
static const int TEXT_TARGET = 0x963;
static const int TEXT_FASTER = 0x964;
static const int TEXT_SLOWER = 0x965;

static const int FONT_HEADING = 0x1c;
static const int FONT_NAME = 0x1d;
static const int FONT_LABEL = 0x1f;
static const int FONT_TIME = 0x20;
static const int FONT_TARGET = 0x21;
static const int FONT_TARGET_LABEL = 0x22;

static const int ANCHOR_TOP_LEFT = 9;
static const int ANCHOR_BASELINE_LEFT = 0x41;
static const int ANCHOR_BASELINE_RIGHT = 0x44;
static const int ANCHOR_CENTER = 0x12;

static const int TITLE_POS = 0x28;
static const int NAME_EXTRA_Y = 0x2a;
static const int ROW_Y = 0x24;
static const int ROW_EXTRA_Y = 0x26;
static const int TIME_RIGHT = 0xf;
static const int LABEL_INSET = 0x13;
static const int LABEL_GAP = 4;
static const int TIME_DECIMALS = 2;

static const int CHALLENGE_OFFSCREEN = -3000;

namespace UI {

static const WindowResult RESULT_CHALLENGE = static_cast<WindowResult>(3);

bool ChallengeEndOfLevelPrompt::pointerPressed(int x, int y, int id) {
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

bool ChallengeEndOfLevelPrompt::pointerDragged(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    EndOfLevelPrompt::pointerDragged(x, y, id);
    if (!m_challenge.contains(x, y) && m_challenge.isPressed()) {
        m_challenge.unpress();
    }
    return false;
}

ChallengeEndOfLevelPrompt::~ChallengeEndOfLevelPrompt() {}

void ChallengeEndOfLevelPrompt::render(midp::Graphics* g, int originY, int originX) {
    if (m_hidden) {
        return;
    }
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    m_continue.render(g, originY, originX);
    m_restart.render(g, originY, originX);
    text->drawString(g, TEXT_CHALLENGE, FONT_HEADING, TITLE_POS, TITLE_POS, ANCHOR_TOP_LEFT);
    int nameY = text->getLineHeight(FONT_HEADING) + NAME_EXTRA_Y;
#ifdef MEDECOMP_PORT
    midp::ReferenceCountedPointer<midp::String> name(medecomp_player_level_title());
    if (name) {
        text->drawString(g, (midp::String*)name, FONT_NAME, TITLE_POS, nameY, ANCHOR_TOP_LEFT);
    } else {
        text->drawString(g, level->getName(), FONT_NAME, TITLE_POS, nameY, ANCHOR_TOP_LEFT);
    }
#else
    text->drawString(g, level->getName(), FONT_NAME, TITLE_POS, nameY, ANCHOR_TOP_LEFT);
#endif

    int timeX = m_width - TIME_RIGHT;
    text->drawString(g, m_time, FONT_TIME, timeX, ROW_Y, ANCHOR_BASELINE_RIGHT);
    int timeWidth = text->getStringWidth(m_time, FONT_TIME);
    text->drawString(g, TEXT_TIME, FONT_LABEL, (m_width - LABEL_INSET) - timeWidth, ROW_Y,
                     ANCHOR_BASELINE_RIGHT);

    timeWidth = text->getStringWidth(m_time, FONT_TIME);
    int targetX = (m_width - TIME_RIGHT) - timeWidth;
    int targetY = text->getLineHeight(FONT_TARGET) + ROW_EXTRA_Y;
    text->drawString(g, m_target, FONT_TARGET, targetX, targetY, ANCHOR_BASELINE_LEFT);
    text->drawString(g, TEXT_TARGET, FONT_TARGET_LABEL, targetX - LABEL_GAP, targetY, ANCHOR_BASELINE_RIGHT);

    int resultText = m_won ? TEXT_FASTER : TEXT_SLOWER;
    text->drawString(g, resultText, FONT_HEADING, m_width >> 1, m_height >> 1, ANCHOR_CENTER);
    m_challenge.render(g, originY, originX);
}

bool ChallengeEndOfLevelPrompt::pointerReleased(int x, int y, int id) {
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

ChallengeEndOfLevelPrompt::ChallengeEndOfLevelPrompt()
    : m_time(NULL),
      m_target(NULL),
      m_challenge(TEXT_CHALLENGE),
      m_hidden(false),
      m_won(false) {
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    int raceTime = canvas->getSceneGame()->getRaceTime();
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendMillisTimeToBuffer(buffer, raceTime, TIME_DECIMALS);
    m_time = buffer->toString();

    int challengeTime = canvas->getChallengeTime();
    buffer = text->clearStringBuffer();
    text->appendMillisTimeToBuffer(buffer, challengeTime, TIME_DECIMALS);
    m_target = buffer->toString();
    m_won = raceTime < challengeTime;

#ifdef MEDECOMP_PORT
    int nextRight = m_next.getX() + m_next.getWidth();
    int nextY = m_next.getY();
#endif
    m_next.setPosition(-m_next.getWidth(), -m_next.getHeight());
    m_challenge.setPosition(CHALLENGE_OFFSCREEN, CHALLENGE_OFFSCREEN);
#ifdef MEDECOMP_PORT
    if (canvas->IsMayhemSessionValid() && medecomp_challenge_allowed()) {
        m_challenge.setPosition(nextRight - m_challenge.getWidth(), nextY);
    }
#endif
}

void ChallengeEndOfLevelPrompt::UnHide() {
    m_hidden = false;
}

}
