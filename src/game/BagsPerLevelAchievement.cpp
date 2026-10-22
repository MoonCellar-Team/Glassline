#include "game/BagsPerLevelAchievement.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

BagsPerLevelAchievement::BagsPerLevelAchievement(int idx, int nameId, int descriptionId, int level, int bags)
    : LevelAchievement(idx, nameId, descriptionId, level), m_bags(bags) {}

midp::StringBuffer* BagsPerLevelAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    midp::ReferenceCountedPointer<midp::String> bags = midp::String::valueOf(m_bags);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, text->getString(level->getName()), bags);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* BagsPerLevelAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    midp::ReferenceCountedPointer<midp::String> bags = midp::String::valueOf(m_bags);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, text->getString(level->getName()), bags);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

int BagsPerLevelAchievement::getBags() const {
    return m_bags;
}

BagsPerLevelAchievement::~BagsPerLevelAchievement() {}
