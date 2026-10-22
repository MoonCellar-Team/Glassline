#include "game/UI/ChapterSelectSpeedrunWindow.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/QuadManager.h"
#include "game/QuadMesh.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"
#include "game/UI/ChapterSelectItem.h"
#include "game/UI/ChapterSelectItemSpeedrun.h"

static const int TEXT_TITLE = 0x819;
static const int TEXT_SUBTITLE = 0x81c;
static const int TEXT_STARS = 0x912;
static const int TEXT_REQUIREMENT = 0x840;
static const int TEXT_TIME = 0x800;
static const int GROUP_STARS = 0x48;
static const int FONT_STAT = 7;
static const int FONT_LABEL = 0xe;
static const int ANCHOR = 0x41;
static const int STAT_NUMBER = 1;
static const int STAR_ON[] = {0x95, 0x96, 0x97};
static const int STAR_OFF[] = {0x98, 0x99, 0x9a};

static const SceneMenu::MenuState STATE_START_SPEEDRUN = static_cast<SceneMenu::MenuState>(22);

namespace UI {

void ChapterSelectSpeedrunWindow::onSelected() {
    ChapterSelectItem* item = static_cast<ChapterSelectItem*>(m_chapters.getSelectedItem());
    const Level* level = item->getLevelObject();
    AppEngine::getLevelData()->setCurrentLevelByUID(LevelData::GAME_MODE_SPEEDRUN, level->getUniqueName());
    AppEngine::getCanvas()->getSceneMenu()->stateTransitionFade(STATE_START_SPEEDRUN);
}

void ChapterSelectSpeedrunWindow::render(midp::Graphics* g, int originY, int originX) {
    ChapterSelectWindow::render(g, originY, originX);
    AppEngine* canvas = AppEngine::getCanvas();
    LevelData* levels = AppEngine::getLevelData();
    const Level* level = levels->getLevel(m_chapters.getSelectedNotch());
    int stars = level->getNumStarsAchieved();
    QuadManager& quads = canvas->getQuadManager();
    for (int i = 0; i < 3; ++i) {
        quads.getMesh(STAR_ON[i])->setVisible(i < stars);
        quads.getMesh(STAR_OFF[i])->setVisible(stars <= i);
    }
    TextManager* text = canvas->getTextManager();
    quads.render(g);
    canvas->drawStatString(g, FONT_STAT, TEXT_STARS, static_cast<AppEngine::StatType>(STAT_NUMBER),
                           levels->calculateNumCollectedStars(), 0x33c, 0x3d, ANCHOR);
    text->drawString(g, TEXT_REQUIREMENT, FONT_LABEL, 0x98, 0x18a, ANCHOR);
    int y = 0x1bb;
    for (int requirement = 3; requirement != 0; --requirement) {
        int millis = level->getSpeedRunRequirementMillis(requirement);
        canvas->drawStatString(g, FONT_STAT, TEXT_TIME, AppEngine::STAT_TIME, millis, 300, y, ANCHOR);
        y += 0x2d;
    }
}

ChapterSelectSpeedrunWindow::ChapterSelectSpeedrunWindow()
    : ChapterSelectWindow(TEXT_SUBTITLE) {
    setTitles(TEXT_TITLE, TEXT_SUBTITLE);
    AppEngine* canvas = AppEngine::getCanvas();
    LevelData* levels = AppEngine::getLevelData();
    int unlocked = levels->getNumUnlockedLevels();
    int levelNum = levels->getLevelNum();
    for (int i = 0; i < levelNum; ++i) {
        const Level* level = levels->getLevel(i);
        if ((i < unlocked || level->isLevelComplete()) && level->getType() != 2) {
            m_chapters.addItem(new ChapterSelectItemSpeedrun(*level));
        }
    }
    canvas->getQuadManager().setGroupVisible(GROUP_STARS, true);
}

ChapterSelectSpeedrunWindow::~ChapterSelectSpeedrunWindow() {
    AppEngine::getCanvas()->getQuadManager().setGroupVisible(GROUP_STARS, false);
}

}
