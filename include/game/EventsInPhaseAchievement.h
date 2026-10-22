#pragma once

#include "game/Achievement.h"

class EventsInPhaseAchievement : public Achievement {
public:
    EventsInPhaseAchievement(int idx, int nameId, int descriptionId, int eventsNeeded);
    virtual ~EventsInPhaseAchievement();

    void phaseOn();
    void phaseOff();
    void eventHappended();

    int m_eventCount;
    int m_eventsNeeded;
};
