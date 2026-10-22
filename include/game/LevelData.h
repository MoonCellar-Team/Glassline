#pragma once

#include "game/Level.h"
#include "game/ModelSet.h"
#include "runtime/midp/array.h"

class GameObject;

namespace midp {
class String;
}

namespace util {
template <class T>
class VectorSE;
}

class LevelData {
public:
    enum GameMode {
        GAME_MODE_STORY = 0,
        GAME_MODE_SPEEDRUN = 1,
        GAME_MODE_CHALLENGE = 2,
        GAME_MODE_MULTIPLAYER_RACE = 3,
        GAME_MODE_MULTIPLAYER_TIMED = 4,
    };

    static const int LOADING_SCREEN_MULTIPLAYER_RACE = 15;
    static const int LOADING_SCREEN_MULTIPLAYER_TIMED = 16;

    static const short STRING_ARRAY[];
    static const int RESOURCE_ARRAY[];
    static const short RUNNER_ANIM_ARRAY[];
    static const short QUAD_ARRAY[];
    static const short MODEL_ARRAY[];

    LevelData();
    ~LevelData();

    void loadData();
    void loadRmsData();
    void saveRmsData();
    void resetRmsData();
    void resetLevelData();

    const ModelSet* getModelSet(int index) const;
    const Level* getLevel(int index) const;
    int getLevelNum() const;
    const Level* getCurrentLevelObject() const;
    int getCurrentLevelIndex() const;
    void setCurrentLevelByIndex(GameMode mode, int index);
    void setCurrentLevelByUID(GameMode mode, const midp::String* uid);
    int getGameMode() const;
    bool isRaceMode() const;
    bool isMultiplayerMode() const;
    int getLevelTimeLimitMillis() const;
    void setLevelTimeLimitMillis(int millis);
    bool isMultiplayerCombatAllowed() const;
    void setMultiplayerCombatAllowed(bool allowed);
    int getCurrentLoadingScreen();

    int getNumUnlockedLevels();
    int getNextStoryLevel(int index);
    void nextStoryLevel();
    bool isCurrentLevelLastStory();
    bool isStoryLevelComplete();
    int calcNextContinueLevel();
    int calculateNumCollectedStars() const;
    bool setLevelComplete(int timeMillis, util::VectorSE<GameObject>& bags);
    bool isGameNewlyCompleted();
    void setGameNotNewlyCompleted();

    midp::array<ModelSet> m_modelSets;
    midp::array<Level> m_levels;
    int m_numUnlockedLevels;
    int m_gameMode;
    int m_currentLevel;
    int m_lastCompletedStoryLevel;
    bool m_gameNewlyCompleted;
    int m_levelTimeLimitMillis;
    bool m_multiplayerCombatAllowed;
};
