#pragma once

#include <vector>

#include "game/AchievementMetrics.h"
#include "runtime/midp/array.h"

class Achievement;
class BagsPerLevelAchievement;
class CountAchievement;
class EventsInPhaseAchievement;
class LevelAchievement;
class TimedAchievement;
class TimedEventsAchievement;
class TimedLevelAchievement;

class AchievementData {
public:
    enum Type {
        TYPE_LEVEL = 0,
        TYPE_TIMED_LEVEL = 1,
        TYPE_BAGS_PER_LEVEL = 2,
        TYPE_BAGS = 3,
        TYPE_STARS = 4,
        TYPE_DEATHS = 5,
        TYPE_BAD_LANDINGS = 6,
        TYPE_UNUSED_7 = 7,
        TYPE_RUN_DIST = 8,
        TYPE_WALL_RUN_DIST = 9,
        TYPE_CLIMB_DIST = 10,
        TYPE_ZIPLINE_DIST = 11,
        TYPE_SLIDE_DIST = 12,
        TYPE_BALANCE_DIST = 13,
        TYPE_FALL_DIST = 14,
        TYPE_SPRINT = 15,
        TYPE_KILLS = 16,
        TYPE_DISARMS = 17,
        TYPE_ENEMY_FALLS = 18,
        TYPE_KILLS_IN_ATTACK = 19,
        TYPE_TIMED_KILLS = 20,
        TYPE_RIVAL_KILLS = 21,
        TYPE_ZIPLINE_KILL = 22,
        TYPE_POPUP = 23,
    };

    static const int ACHIEVEMENT_GAME_COMPLETE = 3;
    static const int ACHIEVEMENT_ALL_BAGS = 6;
    static const int ACHIEVEMENT_ZIPLINE_KILL = 25;

    static const short STRING_ARRAY[];
    static const char* ACHIEVEMENT_FILENAME;

    AchievementData();
    ~AchievementData();

    void loadData();
    void loadRmsData();
    void saveRmsData();
    Achievement* getAchievement(int idx) const;
    int getAchievementNum();
    int getBadgeCount() const;
    LevelAchievement* getLevelAchievement(int level);
    TimedLevelAchievement* getTimedLevelAchievement(int level);

    void registerDeath();
    void registerRunDist(float dist);
    void registerBalanceDist(float dist);
    void registerSlideDist(float dist);
    void registerWallRunDist(float dist);
    void registerZiplineDist(float dist);
    void registerZiplineDrop();
    void registerSprintOn(int raceTime);
    void registerSprintOff();
    void registerClimbDist(float dist);
    void registerFallDist(float dist);
    void registerMapCollision();
    void registerBadLanding();
    void registerAttackStart();
    void registerAttackEnd();
    void registerDisarm();
    void registerStars();
    void registerRivalKill(int raceTime);
    void registerLevelStart(int level);
    bool registerLevelComplete();
    void registerLevelEnd();
    void update(int dt);
    bool registerAchievementComplete(int idx);
    bool registerPopupBox(int text);
    void registerBag();
    void registerKill(int time);
    void registerEnemyFall();
    void registerGameEnd();

    midp::array<Achievement*> m_achievements;
    std::vector<Achievement*> m_otherAchievements;
    std::vector<LevelAchievement*> m_levelAchievements;
    std::vector<TimedLevelAchievement*> m_timedLevelAchievements;
    std::vector<BagsPerLevelAchievement*> m_bagsPerLevelAchievements;
    std::vector<CountAchievement*> m_bagAchievements;
    std::vector<CountAchievement*> m_starAchievements;
    std::vector<CountAchievement*> m_deathAchievements;
    std::vector<CountAchievement*> m_badLandingAchievements;
    std::vector<CountAchievement*> m_unk74;
    std::vector<CountAchievement*> m_runDistAchievements;
    std::vector<CountAchievement*> m_wallRunDistAchievements;
    std::vector<CountAchievement*> m_climbDistAchievements;
    std::vector<CountAchievement*> m_ziplineDistAchievements;
    std::vector<CountAchievement*> m_balanceDistAchievements;
    std::vector<CountAchievement*> m_slideDistAchievements;
    std::vector<CountAchievement*> m_fallDistAchievements;
    std::vector<CountAchievement*> m_killAchievements;
    std::vector<CountAchievement*> m_disarmAchievements;
    std::vector<CountAchievement*> m_rivalKillAchievements;
    std::vector<CountAchievement*> m_enemyFallAchievements;
    std::vector<EventsInPhaseAchievement*> m_killsInAttackAchievements;
    std::vector<TimedEventsAchievement*> m_timedKillAchievements;
    std::vector<TimedAchievement*> m_sprintAchievements;
    std::vector<Achievement*> m_popupAchievements;
    bool m_levelRunning;
    int m_currentLevel;
    int m_gameEndCount;
    AchievementMetrics m_levelMetrics;
    AchievementMetrics m_totalMetrics;
};
