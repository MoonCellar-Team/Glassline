#include "game/UI/ChapterSelectStoryWindow.h"

#include <vector>

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/SceneMenu.h"
#include "game/UI/ChapterSelectItemStory.h"

static const int TEXT_SUBTITLE = 0x817;

static const SceneMenu::MenuState STATE_START_STORY = static_cast<SceneMenu::MenuState>(22);

namespace UI {

void ChapterSelectStoryWindow::onSelected() {
    ChapterSelectItem* item = static_cast<ChapterSelectItem*>(m_chapters.getSelectedItem());
    const Level* level = item->getLevelObject();
    AppEngine::getLevelData()->setCurrentLevelByUID(LevelData::GAME_MODE_STORY, level->getUniqueName());
    AppEngine::getCanvas()->getSceneMenu()->stateTransitionFade(STATE_START_STORY);
}

ChapterSelectStoryWindow::ChapterSelectStoryWindow() : ChapterSelectWindow(TEXT_SUBTITLE) {
    LevelData* levels = AppEngine::getLevelData();
    int levelNum = levels->getLevelNum();
    int unlocked = levels->getNumUnlockedLevels() + 1;
    int count = std::min(levelNum, unlocked);
    for (int i = 0; i < count; ++i) {
        const Level* level = levels->getLevel(i);
        if (static_cast<unsigned>(level->getType()) > 1u) {
            continue;
        }
        m_chapters.addItem(new ChapterSelectItemStory(*level));
    }
}

ChapterSelectStoryWindow::~ChapterSelectStoryWindow() {}

}
