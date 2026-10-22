#include "game/GameUI/EndOfLevelResults.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/MEdgeMap.h"
#include "game/QuadManager.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
extern "C" int medecomp_solo_bags(void);
extern "C" midp::StringBuffer* medecomp_solo_bags_best_text(const Level* level, int timeLimit);
extern "C" midp::String* medecomp_player_level_title(void);
static const int BEST_DY = 60;
#endif

static const int QUAD_MESH = 121;
static const int TEXT_CONTINUE = 0x814;
static const int EDGE_MARGIN = 30;
static const int QUAD_ANCHOR = 12;

static const int TEXT_WIN = 0x947;
static const int TEXT_DRAW = 0x948;
static const int TEXT_LOSS = 0x949;
static const int TEXT_RACE = 0x941;
static const int TEXT_TIMED = 0x942;
static const int TEXT_MINE = 0x945;
static const int TEXT_THEIRS = 0x946;

static const int FONT_MINE = 35;
static const int FONT_THEIRS = 36;
static const int FONT_WIN = 37;
static const int FONT_TITLE = 38;
static const int FONT_SUB = 36;
static const int FONT_NAME = 35;
static const int ANCHOR_CENTER = 18;
static const int ANCHOR_SUB = 33;
static const int ANCHOR_STAT = 12;
static const int SUB_POS = 45;
static const int NAME_GAP = 55;
static const int STAT_DX = 66;
static const int QUAD_GROUP = 0;
static const int QUAD_SCOPE = 2;

namespace GameUI {

EndOfLevelResults::EndOfLevelResults(SceneGame* scene, GameScreen* screen)
    : m_scene(scene),
      m_screen(screen),
      m_backdrop(0, 0, screen->getLocalWidth(), screen->getLocalHeight(), -1),
      m_quad(QUAD_MESH),
      m_button(Button::QUAD_GROUP, Button::QUAD_MESH, TEXT_CONTINUE) {
    int width = screen->getLocalWidth();
    int height = screen->getLocalHeight();
    m_backdrop.setBounds(0, 0, width, height);
    m_button.setTopLeft(width - EDGE_MARGIN - m_button.getWidth(), height - EDGE_MARGIN - m_button.getHeight());
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        m_quad.set(width - EDGE_MARGIN, EDGE_MARGIN, QUAD_ANCHOR);
    }
}

int EndOfLevelResults::pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen) {
    return m_button.pointerReleased(pressX, pressY, x, y, m_screen) ? 1 : 0;
}

void EndOfLevelResults::pointerDragged(int pressX, int pressY, int x, int y) {
    m_button.pointerDragged(pressX, pressY, x, y);
}

void EndOfLevelResults::pointerPressed(int x, int y) {
    m_button.pointerPressed(x, y);
}

void EndOfLevelResults::render2D(midp::Graphics* g) {
    int width = m_screen->getLocalWidth();
    int height = m_screen->getLocalHeight();
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    LevelData* levels = AppEngine::getLevelData();
    const Level* level = levels->getCurrentLevelObject();
    GameObjectPlayer* player = m_screen->getPlayerObject();

    int titleId;
    int subtitleId;
    int mode = levels->getGameMode();
    if (mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        int mine = player->getNumCollectablesCollected();
        SceneGame* scene = m_scene;
        int theirs;
#ifdef MEDECOMP_PORT
        if (scene->numGameScreens() == 1) {
            theirs = port::net::rivalsRemoteBags();
        } else
#endif
        {
            GameScreen* otherScreen = scene->getGameScreen(player->getPlayerIndex() ^ 1);
            GameObjectPlayer* other = otherScreen->getPlayerObject();
            theirs = other->getNumCollectablesCollected();
        }
        int total = scene->getMap().getNumCollectablesTotal();
#ifdef MEDECOMP_PORT
        if (medecomp_solo_bags()) {
            canvas->drawOfStatString(g, FONT_TITLE, TEXT_MINE, mine, total, width >> 1, height >> 1, ANCHOR_CENTER);
            text->drawString(g, medecomp_solo_bags_best_text(NULL, 0), FONT_SUB, width >> 1, (height >> 1) + BEST_DY,
                             ANCHOR_CENTER);
            text->drawString(g, TEXT_TIMED, FONT_SUB, SUB_POS, SUB_POS, ANCHOR_SUB);
            int nameX = text->getStringWidth(TEXT_TIMED, FONT_NAME) + NAME_GAP;
            midp::ReferenceCountedPointer<midp::String> name(medecomp_player_level_title());
            if (name) {
                text->drawString(g, (midp::String*)name, FONT_NAME, nameX, SUB_POS, ANCHOR_SUB);
            } else {
                text->drawString(g, level->getName(), FONT_NAME, nameX, SUB_POS, ANCHOR_SUB);
            }
            m_button.render2D(g);
            return;
        }
#endif
        if (mine < theirs) {
            titleId = TEXT_LOSS;
        } else if (theirs < mine) {
            titleId = TEXT_WIN;
        } else {
            titleId = TEXT_DRAW;
        }
        int statX = m_quad.getX() - STAT_DX;
        int statY = m_quad.getY();
        canvas->drawOfStatString(g, FONT_MINE, TEXT_MINE, mine, total, statX, statY + 2, ANCHOR_STAT);
        canvas->drawOfStatString(g, FONT_THEIRS, TEXT_THEIRS, theirs, total, statX, statY + 22, ANCHOR_STAT);
        subtitleId = TEXT_TIMED;
    } else if (mode == LevelData::GAME_MODE_MULTIPLAYER_RACE) {
        titleId = TEXT_WIN;
        if (!player->isVictorious()) {
            titleId = TEXT_LOSS;
        }
        subtitleId = TEXT_RACE;
    } else {
        titleId = TEXT_DRAW;
        subtitleId = -1;
    }

    int titleFont = FONT_TITLE;
    if (titleId == TEXT_WIN) {
        titleFont = FONT_WIN;
    }
    text->drawString(g, titleId, titleFont, width >> 1, height >> 1, ANCHOR_CENTER);
    text->drawString(g, subtitleId, FONT_SUB, SUB_POS, SUB_POS, ANCHOR_SUB);
    int nameId = level->getName();
    int nameX = text->getStringWidth(subtitleId, FONT_NAME) + NAME_GAP;
#ifdef MEDECOMP_PORT
    midp::ReferenceCountedPointer<midp::String> own(medecomp_player_level_title());
    if (own) {
        text->drawString(g, (midp::String*)own, FONT_NAME, nameX, SUB_POS, ANCHOR_SUB);
    } else
#endif
    text->drawString(g, nameId, FONT_NAME, nameX, SUB_POS, ANCHOR_SUB);
    m_button.render2D(g);
}

void EndOfLevelResults::render3D(m3g::Graphics3D* g3d) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
#ifdef MEDECOMP_PORT
    bool dimmed = m_scene->numGameScreens() == 1;
#ifdef MEDECOMP_SPLIT_STACKED
    dimmed = true;
#endif
    if (dimmed) {
        m_screen->m_dimRect.render3D(g3d);
    } else
#endif
    {
        quads.setGroupVisible(QUAD_GROUP, true);
        quads.render(g3d, QUAD_SCOPE);
        quads.setGroupVisible(QUAD_GROUP, false);
    }
    m_button.render3D(g3d);
    m_quad.render3D(g3d, quads);
}

EndOfLevelResults::~EndOfLevelResults() {}

}
