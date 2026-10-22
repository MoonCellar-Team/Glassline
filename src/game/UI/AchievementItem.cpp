#include "game/UI/AchievementItem.h"

#include "game/Achievement.h"
#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "game/UI/AchievementsList.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int FONT_NAME = 0x14;
static const int FONT_DESCRIPTION = 0x15;
static const int TEXT_ANCHOR = 9;
static const int TEXT_X = 0xb;
static const int TEXT_Y = 6;
static const int ROW_WIDTH = 0x29c;
static const int DESCRIPTION_WIDTH = 0x286;
static const int ITEM_WIDTH = 0x2eb;
static const int GROUP_BADGE = 0x39;
static const int MESH_INCOMPLETE = 0x93;
static const int MESH_COMPLETE = 0x94;
static const int BADGE_X = 0x2c9;
static const int RENDER_SCOPE = 2;
static const int COLOR_ROW = 0xffffff;

namespace UI {

void AchievementItem::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int x = m_x + originX;
    int y = m_y + originY;
    int clipX = g->getClipX();
    int clipY = g->getClipY();
    int clipW = g->getClipWidth();
    int clipH = g->getClipHeight();
    if (x > clipX + clipW || clipX > x + m_width) {
        return;
    }
    if (y > clipY + clipH || clipY > y + m_height) {
        return;
    }
    g->setColor(COLOR_ROW);
    g->fillRect(x, y, ROW_WIDTH, m_height);
    text->drawString(g, m_name, FONT_NAME, originX + TEXT_X + m_x, originY + TEXT_Y + m_y, TEXT_ANCHOR);
    int nameHeight = text->getLineHeight(FONT_NAME);
    m_description.draw(g, originX + TEXT_X + m_x, originY + TEXT_Y + m_y + nameHeight, TEXT_ANCHOR);

    AchievementsList* list = static_cast<AchievementsList*>(m_parent);
    Achievement* achievement = list->getAchievementData()->getAchievement(m_index);
    QuadManager* quads = m_quadManager;
    quads->setGroupVisible(GROUP_BADGE, true);
    float badgeX = static_cast<float>(originX + m_x + BADGE_X);
    float badgeY = static_cast<float>(m_y + originY + (m_height >> 1));
    quads->setGroupPosition(GROUP_BADGE, badgeX, badgeY);
    bool complete = achievement->isComplete() != 0;
    quads->setMeshVisible(MESH_INCOMPLETE, !complete);
    quads->setMeshVisible(MESH_COMPLETE, complete);
    quads->render(g, RENDER_SCOPE);
    quads->setGroupVisible(GROUP_BADGE, false);
    g->setClip(clipX, clipY, clipW, clipH);
}

AchievementItem::AchievementItem(AchievementsList* list, int index)
    : WindowElement(0, 0, ITEM_WIDTH, 0),
      m_index(index),
      m_name(NULL) {
    m_parent = list;
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    Achievement* achievement = list->getAchievementData()->getAchievement(index);
    m_name = achievement->getNameStringBuffer()->toString();
    m_description.wrapString(achievement->getDescriptionStringBuffer()->toString(), FONT_DESCRIPTION,
                             DESCRIPTION_WIDTH);
    int lines = m_description.getNumWrappedLines();
    int height = text->getLineHeight(FONT_DESCRIPTION) * lines + text->getLineHeight(FONT_NAME) + TEXT_Y;
    setHeight(height);
}

AchievementItem::~AchievementItem() {}

}
