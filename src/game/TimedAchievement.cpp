#include "game/TimedAchievement.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

void TimedAchievement::endTimer() {
    m_startTime = -1.0f;
}

TimedAchievement::TimedAchievement(int idx, int nameId, int descriptionId, int seconds)
    : Achievement(idx, nameId, descriptionId), m_startTime(-1.0f), m_seconds(seconds) {}

midp::StringBuffer* TimedAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* TimedAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

void TimedAchievement::runTimer(int time) {
    float start = m_startTime;
    if (start == -1.0f) {
        m_startTime = (float)time;
        return;
    }
    if (!isComplete() && (float)time - start >= (float)m_seconds) {
        AppEngine::getAchievementData()->registerAchievementComplete(m_idx);
    }
}

TimedAchievement::~TimedAchievement() {}
