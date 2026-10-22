#include "game/TimedLevelAchievement.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

TimedLevelAchievement::TimedLevelAchievement(int idx, int nameId, int descriptionId, int level, int seconds)
    : LevelAchievement(idx, nameId, descriptionId, level), m_seconds(seconds) {}

midp::StringBuffer* TimedLevelAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, text->getString(level->getName()), seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* TimedLevelAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getLevel(m_level);
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, text->getString(level->getName()), seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

int TimedLevelAchievement::getTime() const {
    return m_seconds * 1000;
}

TimedLevelAchievement::~TimedLevelAchievement() {}
