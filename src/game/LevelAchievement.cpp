#include "game/LevelAchievement.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/TextManager.h"

LevelAchievement::LevelAchievement(int idx, int nameId, int descriptionId, int level)
    : Achievement(idx, nameId, descriptionId), m_level(level) {}

midp::StringBuffer* LevelAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, text->getString(level->getName()));
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* LevelAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, text->getString(level->getName()));
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

int LevelAchievement::getLevel() {
    return m_level;
}

LevelAchievement::~LevelAchievement() {}
