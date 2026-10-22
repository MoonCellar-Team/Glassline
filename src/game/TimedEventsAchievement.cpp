#include "game/TimedEventsAchievement.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

void TimedEventsAchievement::levelStarted() {
    int count = m_eventTimes.m_length;
    for (int i = 0; i != count; i++) {
        m_eventTimes[i] = -1;
    }
}

TimedEventsAchievement::TimedEventsAchievement(int idx, int nameId, int descriptionId, int events, int seconds)
    : Achievement(idx, nameId, descriptionId), m_seconds(seconds), m_eventTimes(events) {}

midp::StringBuffer* TimedEventsAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> events = midp::String::valueOf(m_eventTimes.m_length);
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, events, seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* TimedEventsAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> events = midp::String::valueOf(m_eventTimes.m_length);
    midp::ReferenceCountedPointer<midp::String> seconds = midp::String::valueOf(m_seconds);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, events, seconds);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

void TimedEventsAchievement::eventHappended(int time) {
    if (isComplete()) {
        return;
    }
    int last = m_eventTimes.m_length - 1;
    for (int i = 0; i != last; i++) {
        m_eventTimes[i] = m_eventTimes[i + 1];
    }
    m_eventTimes[last] = time;
    int first = m_eventTimes[0];
    if (first != -1 && time - first <= m_seconds) {
        AppEngine::getAchievementData()->registerAchievementComplete(m_idx);
    }
}

TimedEventsAchievement::~TimedEventsAchievement() {}
