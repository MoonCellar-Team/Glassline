#include "game/EventsInPhaseAchievement.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"

void EventsInPhaseAchievement::phaseOn() {
    if (m_eventCount == -1) {
        m_eventCount = 0;
    }
}

void EventsInPhaseAchievement::phaseOff() {
    m_eventCount = -1;
}

EventsInPhaseAchievement::EventsInPhaseAchievement(int idx, int nameId, int descriptionId, int eventsNeeded)
    : Achievement(idx, nameId, descriptionId), m_eventCount(-1), m_eventsNeeded(eventsNeeded) {}

void EventsInPhaseAchievement::eventHappended() {
    if (isComplete() || m_eventCount == -1) {
        return;
    }
    if (++m_eventCount == m_eventsNeeded) {
        AppEngine::getAchievementData()->registerAchievementComplete(m_idx);
    }
}

EventsInPhaseAchievement::~EventsInPhaseAchievement() {}
