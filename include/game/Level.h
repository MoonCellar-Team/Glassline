#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

namespace midp {
class DataInputStream;
class DataOutputStream;
class String;
}

class Level {
public:
    static const int NUM_STAR_TIMES = 3;
    static const int MAX_BAGS = 19;

    Level();
    Level(const Level& other);
    ~Level();

    void levelReset();
    void resetRms(bool resetComplete);
    void writeRms(midp::DataOutputStream* out);
    bool readRms(midp::DataInputStream& in);

    midp::String* getUniqueName() const;
    int getType() const;
    int getName() const;
    int getObjective() const;
    int getDateTime() const;
    int getMapResId() const;
    int getSpeedRunRequirementMillis(int stars) const;
    int getLoadingScreen() const;
    int getStartMusicId() const;
    int getIntroFaithAnim(int playerIndex) const;
    int getIntroCamAnim(int playerIndex) const;
    int getCompletionBackground() const;
    bool isLevelComplete() const;
    int getBestSpeedRunTimeMillis() const;
    int getMinStarsWithTime(int timeMillis) const;
    int getNumStarsAchieved() const;
    bool isBagFound(int index) const;
    int getNumBagsFound() const;
    int getNumTotalBags() const;

    midp::ReferenceCountedPointer<midp::String> m_uniqueName;
    int m_type;
    int m_name;
    int m_objective;
    int m_dateTime;
    int m_mapResId;
    int m_mapResIdMultiplayer;
    int m_speedRunRequirementMillis[NUM_STAR_TIMES];
    int m_loadingScreen;
    int m_startMusicId;
    int m_introFaithAnim;
    int m_introCamAnim;
    int m_introFaithAnimMultiplayer[2];
    int m_introCamAnimMultiplayer[2];
    int m_completionBackground;
    bool m_complete;
    int m_bestSpeedRunTimeMillis;
    midp::array<bool> m_bagsFound;
};
