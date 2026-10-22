#include "game/UI/AchievementsList.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/UI/AchievementItem.h"

static const int ITEM_X = 2;
static const int ITEM_GAP = 16;

namespace UI {

void AchievementsList::render(midp::Graphics* g, int originY, int originX) {
    Window::render(g, originY, originX);
}

AchievementsList::AchievementsList(int x, int y, int width, int height)
    : Window(x, y, width, height),
      m_achievementData(AppEngine::getCanvas()->getAchievementData()) {
    m_marginX = 0;
    m_marginY = 0;
    adjustClientArea();
    int count = m_achievementData->getAchievementNum();
    int rowY = 0;
    for (int i = 0; i < count; ++i) {
        AchievementItem* item = new AchievementItem(this, i);
        item->setPosition(ITEM_X, rowY);
        addElement(item);
        rowY += item->getHeight() + ITEM_GAP;
    }
}

AchievementData* AchievementsList::getAchievementData() const {
    return m_achievementData;
}

AchievementsList::~AchievementsList() {}

}
