#include "game/UI/ChapterSelectMultiplayerWindow.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"
#include "game/UI/ChapterSelectItemMultiplayer.h"
#include "game/UI/NotchedItemStringId.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
extern "C" bool medecomp_mp_add_player_levels(UI::ChapterArrowedScroller* race, UI::ChapterArrowedScroller* timed);
extern "C" void medecomp_mp_choose(const UI::WindowElement* item);
extern "C" const char* medecomp_mp_item_name(const UI::WindowElement* item);
extern "C" int medecomp_solo_bags_choosing(void);
extern "C" void medecomp_solo_bags_chosen(const Level* level, int timeLimit);
extern "C" midp::StringBuffer* medecomp_solo_bags_item_best(const UI::WindowElement* item, int timeLimit);
extern "C" int medecomp_mp_item_new_limit(const UI::WindowElement* item);
static const int TEXT_SOLO = 0x9cd;
static bool modeShown() { return !medecomp_solo_bags_choosing(); }
#endif

static const int TEXT_TITLE = 0x819;
static const int TEXT_SUBTITLE = 0x930;
static const int TEXT_START = 0x82e;
static const int TEXT_RACE = 0x941;
static const int TEXT_TIMED = 0x942;
static const int TEXT_TIME_2MIN = 0x94e;
static const int TEXT_TIME_3MIN = 0x94f;
static const int TEXT_TIME_5MIN = 0x950;
static const int TEXT_COMBAT = 0x952;

static const int TIME_2MIN_MILLIS = 120000;
static const int TIME_3MIN_MILLIS = 180000;
static const int TIME_5MIN_MILLIS = 300000;

static const int LEVEL_TYPE_RACE = 1;
static const int LEVEL_TYPE_TIMED = 2;

static const int FONT_LABEL = 14;
static const int ANCHOR_LABEL = 12;
static const int LABEL_OFFSET_X = 20;
static const int LABEL_OFFSET_Y = 9;

static const int BACKGROUND_POS = 150;
static const int MODE_X = 150;
static const int MODE_Y = 648;
static const int MODE_NOTCH_WIDTH = 300;
static const int CHAPTER_X = 150;
static const int CHAPTER_Y = 160;
static const int CHAPTER_NOTCH_WIDTH = 724;
static const int OPTION_X = 400;
static const int COMBAT_Y = 220;
static const int TIME_2MIN_Y = 320;
static const int TIME_3MIN_Y = 370;
static const int TIME_5MIN_Y = 420;
static const int START_GAP = 16;

static const SceneMenu::MenuState STATE_START_MULTIPLAYER =
    static_cast<SceneMenu::MenuState>(22);

namespace UI {

bool ChapterSelectMultiplayerWindow::isRaceMode() const {
    WindowElement* item = m_modeScroller.getSelectedItem();
    if (item == NULL) {
        return false;
    }
    return static_cast<NotchedItemStringId*>(item)->getStringId() == TEXT_RACE;
}

int ChapterSelectMultiplayerWindow::getRivalTimeMillis() const {
    if (m_time2Min.getChecked()) {
        return TIME_2MIN_MILLIS;
    }
    if (m_time3Min.getChecked()) {
        return TIME_3MIN_MILLIS;
    }
    if (m_time5Min.getChecked()) {
        return TIME_5MIN_MILLIS;
    }
    return 0;
}

bool ChapterSelectMultiplayerWindow::getCombatAllowed() const {
    return m_combat.getChecked();
}

bool ChapterSelectMultiplayerWindow::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
#ifdef MEDECOMP_PORT
    if (modeShown())
#endif
    m_modeScroller.pointerPressed(x, y, id);
    ChapterArrowedScroller& chapters = isRaceMode() ? m_raceScroller : m_timedScroller;
    chapters.pointerPressed(x, y, id);
    if (m_startButton.contains(x, y)) {
        m_startButton.pointerPressed(x, y, id);
    }
    return true;
}

bool ChapterSelectMultiplayerWindow::pointerDragged(int x, int y, int id) {
    TitledWindow::pointerDragged(x, y, id);
#ifdef MEDECOMP_PORT
    if (modeShown())
#endif
    m_modeScroller.pointerDragged(x, y, id);
    ChapterArrowedScroller& chapters = isRaceMode() ? m_raceScroller : m_timedScroller;
    chapters.pointerDragged(x, y, id);
    if (!m_startButton.contains(x, y) && !m_startButton.isPressed()) {
        m_startButton.unpress();
    }
    return true;
}

void ChapterSelectMultiplayerWindow::update(int dt) {
    TitledWindow::update(dt);
    m_modeScroller.update(dt);
    if (m_hasRaceLevels) {
        m_raceScroller.update(dt);
    }
    m_timedScroller.update(dt);
#ifdef MEDECOMP_PORT
    if (medecomp_solo_bags_choosing()) {
        int limit = medecomp_mp_item_new_limit(m_timedScroller.getSelectedItem());
        if (limit > 0) {
            m_time2Min.setChecked(limit == TIME_2MIN_MILLIS);
            m_time3Min.setChecked(limit == TIME_3MIN_MILLIS);
            m_time5Min.setChecked(limit == TIME_5MIN_MILLIS);
        }
    }
    if (port::net::raceOpponent() && port::net::raceIsHost()) {
        ChapterArrowedScroller& chapters = isRaceMode() ? m_raceScroller : m_timedScroller;
        ChapterSelectItem* item = static_cast<ChapterSelectItem*>(chapters.getSelectedItem());
        port::net::raceBrowsing(isRaceMode(), item ? item->getLevelObject() : NULL, getRivalTimeMillis(),
                                getCombatAllowed(), medecomp_mp_item_name(item));
    }
#endif
}

void ChapterSelectMultiplayerWindow::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    m_startButton.render(g, originY, originX);
#ifdef MEDECOMP_PORT
    if (modeShown())
#endif
    m_modeScroller.render(g, originY, originX);

    int labelX = m_combat.getX() - LABEL_OFFSET_X;
    TextManager* text = AppEngine::getCanvas()->getTextManager();
#ifdef MEDECOMP_PORT
    if (medecomp_solo_bags_choosing()) {
        midp::StringBuffer* best = medecomp_solo_bags_item_best(m_timedScroller.getSelectedItem(), getRivalTimeMillis());
        text->drawString(g, best, FONT_LABEL, labelX, m_combat.getY() + LABEL_OFFSET_Y, ANCHOR_LABEL);
    } else
#endif
    {
        m_combat.render(g, originY, originX);
        text->drawString(g, TEXT_COMBAT, FONT_LABEL, labelX, m_combat.getY() + LABEL_OFFSET_Y, ANCHOR_LABEL);
    }

    if (!isRaceMode()) {
        m_timedScroller.render(g, originY, originX);
        m_time2Min.render(g, originY, originX);
        m_time3Min.render(g, originY, originX);
        m_time5Min.render(g, originY, originX);
        text->drawString(g, TEXT_TIME_2MIN, FONT_LABEL, labelX, m_time2Min.getY() + LABEL_OFFSET_Y,
                         ANCHOR_LABEL);
        text->drawString(g, TEXT_TIME_3MIN, FONT_LABEL, labelX, m_time3Min.getY() + LABEL_OFFSET_Y,
                         ANCHOR_LABEL);
        text->drawString(g, TEXT_TIME_5MIN, FONT_LABEL, labelX, m_time5Min.getY() + LABEL_OFFSET_Y,
                         ANCHOR_LABEL);
    } else {
        m_raceScroller.render(g, originY, originX);
    }
}

bool ChapterSelectMultiplayerWindow::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
#ifdef MEDECOMP_PORT
    if (modeShown())
#endif
    m_modeScroller.pointerReleased(x, y, id);
    if (!isRaceMode()) {
        m_timedScroller.pointerReleased(x, y, id);
        if (!m_time2Min.getChecked() && m_time2Min.contains(x, y)) {
            m_time2Min.pointerReleased(x, y, id);
            m_time3Min.setChecked(false);
            m_time5Min.setChecked(false);
        }
        if (!m_time3Min.getChecked() && m_time3Min.contains(x, y)) {
            m_time3Min.pointerReleased(x, y, id);
            m_time2Min.setChecked(false);
            m_time5Min.setChecked(false);
        }
        if (!m_time5Min.getChecked() && m_time5Min.contains(x, y)) {
            m_time5Min.pointerReleased(x, y, id);
            m_time2Min.setChecked(false);
            m_time3Min.setChecked(false);
        }
    } else {
        m_raceScroller.pointerReleased(x, y, id);
    }
    if (m_combat.contains(x, y)) {
        m_combat.pointerReleased(x, y, id);
    }
    if (m_startButton.contains(x, y)) {
        m_startButton.pointerReleased(x, y, id);
#ifdef MEDECOMP_PORT
        if (!port::net::raceOpponent() && !port::net::splitScreenChosen() && !medecomp_solo_bags_choosing()) {
            close(RESULT_OK);
            return true;
        }
#endif
        onSelected();
        close(RESULT_NONE);
    } else if (m_startButton.isPressed()) {
        m_startButton.unpress();
    }
    return true;
}

void ChapterSelectMultiplayerWindow::onSelected() {
    LevelData* levels = AppEngine::getLevelData();
    if (!isRaceMode()) {
        ChapterSelectItem* item =
            static_cast<ChapterSelectItem*>(m_timedScroller.getSelectedItem());
        const Level* level = item->getLevelObject();
#ifdef MEDECOMP_PORT
        medecomp_mp_choose(item);
#endif
        levels->setCurrentLevelByUID(LevelData::GAME_MODE_MULTIPLAYER_TIMED, level->getUniqueName());
        levels->setLevelTimeLimitMillis(getRivalTimeMillis());
#ifdef MEDECOMP_PORT
        if (medecomp_solo_bags_choosing()) {
            medecomp_solo_bags_chosen(level, getRivalTimeMillis());
        }
#endif
    } else {
        ChapterSelectItem* item =
            static_cast<ChapterSelectItem*>(m_raceScroller.getSelectedItem());
        const Level* level = item->getLevelObject();
#ifdef MEDECOMP_PORT
        medecomp_mp_choose(item);
        if (port::net::raceOpponent()) {
            levels->setCurrentLevelByUID(LevelData::GAME_MODE_SPEEDRUN, level->getUniqueName());
        } else
#endif
        levels->setCurrentLevelByUID(LevelData::GAME_MODE_MULTIPLAYER_RACE, level->getUniqueName());
    }
    levels->setMultiplayerCombatAllowed(getCombatAllowed());
#ifdef MEDECOMP_PORT
    if (medecomp_solo_bags_choosing()) {
        levels->setMultiplayerCombatAllowed(false);
    }
#endif
    AppEngine::getCanvas()->getSceneMenu()->stateTransitionFade(STATE_START_MULTIPLAYER);
}

ChapterSelectMultiplayerWindow::~ChapterSelectMultiplayerWindow() {}

ChapterSelectMultiplayerWindow::ChapterSelectMultiplayerWindow()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_hasRaceLevels(false),
      m_startButton(TEXT_START) {
    LevelData* levels = AppEngine::getLevelData();
    setShowBackground(false);
    m_background.setPosition(BACKGROUND_POS, BACKGROUND_POS);
    m_background.setDimensions(0, 0);
    m_startButton.setPosition(m_backButton.getX() - START_GAP - m_startButton.getWidth(),
                              m_backButton.getY());
    m_modeScroller.set(true, MODE_X, MODE_Y, MODE_NOTCH_WIDTH);
    m_timedScroller.set(true, CHAPTER_X, CHAPTER_Y, CHAPTER_NOTCH_WIDTH);
    m_raceScroller.set(false, CHAPTER_X, CHAPTER_Y, CHAPTER_NOTCH_WIDTH);

    m_combat.setPosition(OPTION_X, COMBAT_Y);
    m_combat.setChecked(true);
    m_time2Min.setPosition(OPTION_X, TIME_2MIN_Y);
    m_time3Min.setPosition(OPTION_X, TIME_3MIN_Y);
    m_time5Min.setPosition(OPTION_X, TIME_5MIN_Y);
    m_time3Min.setChecked(true);

    int unlocked = levels->getNumUnlockedLevels();
    int count = levels->getLevelNum();
    bool seenRace = false;
    for (int i = 0; i < count; i++) {
        const Level* level = levels->getLevel(i);
        int type = level->getType();
        if (type == LEVEL_TYPE_TIMED) {
            m_timedScroller.addItem(new ChapterSelectItemMultiplayer(*level));
        } else if (type == LEVEL_TYPE_RACE &&
                   (!seenRace || i < unlocked || level->isLevelComplete())) {
            m_raceScroller.addItem(new ChapterSelectItemMultiplayer(*level));
            seenRace = true;
            m_hasRaceLevels = true;
        }
    }
#ifdef MEDECOMP_PORT
    bool solo = medecomp_solo_bags_choosing();
    if (solo) {
        setTitles(TEXT_TITLE, TEXT_SOLO);
        m_combat.setChecked(false);
        m_hasRaceLevels = false;
    }
    if (medecomp_mp_add_player_levels(solo ? NULL : &m_raceScroller, &m_timedScroller) && !solo) {
        m_hasRaceLevels = true;
    }
#endif
    m_raceScroller.setLooped(m_raceScroller.getItemCount() > 1);

    if (m_hasRaceLevels) {
        m_modeScroller.addItem(new NotchedItemStringId(TEXT_RACE));
        m_modeScroller.addItem(new NotchedItemStringId(TEXT_TIMED));
        m_modeScroller.addItem(new NotchedItemStringId(TEXT_RACE));
        m_modeScroller.addItem(new NotchedItemStringId(TEXT_TIMED));
    } else {
        m_modeScroller.addItem(new NotchedItemStringId(TEXT_TIMED));
    }
}

}
