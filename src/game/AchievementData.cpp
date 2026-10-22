#include "game/AchievementData.h"

#include "ea_online/SpywareManager.h"
#include "game/AppEngine.h"
#include "game/BagsPerLevelAchievement.h"
#include "game/CountAchievement.h"
#include "game/EventsInPhaseAchievement.h"
#include "game/GameObjectPlayer.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/MEdgeMap.h"
#include "game/ResourceManager.h"
#include "game/SceneGame.h"
#include "game/TimedAchievement.h"
#include "game/TimedEventsAchievement.h"
#include "game/TimedLevelAchievement.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

#ifdef MEDECOMP_PORT
extern "C" bool medecomp_achievement_unlocked(int index);
extern "C" void medecomp_achievements_saved(void);
#endif

static const int RESOURCE_ACHIEVEMENT_DATA = 181;

static const int NUM_NON_STORY_LEVELS = 4;

const short AchievementData::STRING_ARRAY[] = {
    2336, 2335, 2340, 2339, 2338, 2337, 2167, 2166, 2169, 2168, 2171, 2170, 2173, 2172,
    2185, 2184, 2187, 2186, 2189, 2188, 2191, 2190, 2193, 2192, 2195, 2194, 2197, 2196,
    2217, 2216, 2199, 2198, 2201, 2200, 2203, 2202, 2207, 2206, 2209, 2208, 2223, 2222,
    2225, 2224, 2211, 2210, 2213, 2212, 2205, 2204, 2215, 2214, 2219, 2218, 2221, 2220,
};

const char* AchievementData::ACHIEVEMENT_FILENAME = "achievements_data";

static void completeReached(AchievementData* data, std::vector<CountAchievement*>& list, int value) {
    for (std::vector<CountAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        CountAchievement* achievement = *it;
        if (achievement->getCount() <= value) {
            data->registerAchievementComplete(achievement->getIdx());
        }
    }
}

template <class T>
static T* addTo(std::vector<T*>& list, T* achievement) {
    list.push_back(achievement);
    return achievement;
}

void AchievementData::update(int dt) {
    if (m_levelRunning) {
        m_levelMetrics.m_timeMillis += dt;
    }
}

void AchievementData::registerDeath() {
    m_levelMetrics.m_deaths++;
}

void AchievementData::registerBadLanding() {
    m_levelMetrics.m_badLandings++;
}

int AchievementData::getBadgeCount() const {
    int count = 0;
    for (int i = 0; i < m_achievements.m_length; i++) {
        if (m_achievements[i]->isComplete()) {
            count++;
        }
    }
    return count;
}

void AchievementData::registerMapCollision() {
    static_cast<EventsInPhaseAchievement*>(m_achievements[ACHIEVEMENT_ZIPLINE_KILL])->phaseOff();
}

void AchievementData::registerZiplineDrop() {
    static_cast<EventsInPhaseAchievement*>(m_achievements[ACHIEVEMENT_ZIPLINE_KILL])->phaseOn();
}

void AchievementData::registerLevelStart(int level) {
    m_currentLevel = level;
    m_levelRunning = true;
    m_levelMetrics = AchievementMetrics();
    std::vector<TimedEventsAchievement*>& list = m_timedKillAchievements;
    for (std::vector<TimedEventsAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        (*it)->levelStarted();
    }
}

void AchievementData::registerAttackEnd() {
    std::vector<EventsInPhaseAchievement*>& list = m_killsInAttackAchievements;
    for (std::vector<EventsInPhaseAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        (*it)->phaseOff();
    }
}

void AchievementData::registerAttackStart() {
    std::vector<EventsInPhaseAchievement*>& list = m_killsInAttackAchievements;
    for (std::vector<EventsInPhaseAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        (*it)->phaseOn();
    }
}

void AchievementData::registerSprintOff() {
    std::vector<TimedAchievement*>& list = m_sprintAchievements;
    for (std::vector<TimedAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        (*it)->endTimer();
    }
}

LevelAchievement* AchievementData::getLevelAchievement(int level) {
    std::vector<LevelAchievement*>& list = m_levelAchievements;
    for (std::vector<LevelAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        LevelAchievement* achievement = *it;
        if (achievement->getLevel() == level) {
            return achievement;
        }
    }
    return NULL;
}

TimedLevelAchievement* AchievementData::getTimedLevelAchievement(int level) {
    std::vector<TimedLevelAchievement*>& list = m_timedLevelAchievements;
    for (std::vector<TimedLevelAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        TimedLevelAchievement* achievement = *it;
        if (achievement->getLevel() == level) {
            return achievement;
        }
    }
    return NULL;
}

AchievementData::AchievementData()
    : m_levelRunning(false), m_currentLevel(0), m_gameEndCount(0), m_levelMetrics(), m_totalMetrics() {}

void AchievementData::saveRmsData() {
    if (AppEngine::getCanvas()->storageFull()) {
        return;
    }
    midp::ReferenceCountedPointer<midp::String> name = new midp::String(ACHIEVEMENT_FILENAME);
    midp::ReferenceCountedPointer<midp::OutputStream> file = midp::OutputStream::getResourceAsStream(name);
    midp::DataOutputStream out(file);
    out.writeInt(m_achievements.m_length);
    m_totalMetrics.write(out);
    for (int i = 0; i < m_achievements.m_length; i++) {
        out.writeBoolean(m_achievements[i]->isComplete());
    }
    out.close();
}

bool AchievementData::registerAchievementComplete(int idx) {
    Achievement* achievement = getAchievement(idx);
    if (achievement->isComplete()) {
        return false;
    }
    achievement->complete();
    SpywareManager::getInstance()->trackAchievedBadge(idx);
#ifdef MEDECOMP_PORT
    if (!medecomp_achievement_unlocked(idx))
#endif
        AppEngine::getCanvas()->notifyAchievementComplete(achievement);
    saveRmsData();
    return true;
}

bool AchievementData::registerPopupBox(int text) {
    std::vector<Achievement*>& list = m_popupAchievements;
    for (std::vector<Achievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        Achievement* achievement = *it;
        if (achievement->getNameId() == text) {
            registerAchievementComplete(achievement->getIdx());
            return true;
        }
    }
    return false;
}

void AchievementData::registerFallDist(float dist) {
    m_levelMetrics.m_fallDist += dist;
    completeReached(this, m_fallDistAchievements, (int)(m_levelMetrics.m_fallDist + m_totalMetrics.m_fallDist));
}

void AchievementData::registerSlideDist(float dist) {
    m_levelMetrics.m_slideDist += dist;
    completeReached(this, m_slideDistAchievements, (int)(m_levelMetrics.m_slideDist + m_totalMetrics.m_slideDist));
}

void AchievementData::registerBalanceDist(float dist) {
    m_levelMetrics.m_balanceDist += dist;
    completeReached(this, m_balanceDistAchievements,
                    (int)(m_levelMetrics.m_balanceDist + m_totalMetrics.m_balanceDist));
}

void AchievementData::registerZiplineDist(float dist) {
    m_levelMetrics.m_ziplineDist += dist;
    completeReached(this, m_ziplineDistAchievements,
                    (int)(m_levelMetrics.m_ziplineDist + m_totalMetrics.m_ziplineDist));
}

void AchievementData::registerClimbDist(float dist) {
    m_levelMetrics.m_climbDist += dist;
    completeReached(this, m_climbDistAchievements, (int)(m_levelMetrics.m_climbDist + m_totalMetrics.m_climbDist));
}

void AchievementData::registerWallRunDist(float dist) {
    m_levelMetrics.m_wallRunDist += dist;
    completeReached(this, m_wallRunDistAchievements,
                    (int)(m_levelMetrics.m_wallRunDist + m_totalMetrics.m_wallRunDist));
}

void AchievementData::registerRunDist(float dist) {
    m_levelMetrics.m_runDist += dist;
    completeReached(this, m_runDistAchievements, (int)(m_levelMetrics.m_runDist + m_totalMetrics.m_runDist));
}

void AchievementData::registerStars() {
    completeReached(this, m_starAchievements, AppEngine::getLevelData()->calculateNumCollectedStars());
}

void AchievementData::registerBag() {
    LevelData* levelData = AppEngine::getLevelData();
    if (levelData->isMultiplayerMode()) {
        return;
    }
    m_levelMetrics.m_bags++;
    std::vector<BagsPerLevelAchievement*>& list = m_bagsPerLevelAchievements;
    for (std::vector<BagsPerLevelAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        BagsPerLevelAchievement* achievement = *it;
        if (achievement->getLevel() == m_currentLevel && achievement->getBags() <= m_levelMetrics.m_bags) {
            registerAchievementComplete(achievement->getIdx());
        }
    }

    int totalBags = 0;
    if (!m_achievements[ACHIEVEMENT_ALL_BAGS]->isComplete()) {
        int current = levelData->getCurrentLevelIndex();
        int numLevels = levelData->getLevelNum() - NUM_NON_STORY_LEVELS;
        bool allFound = true;
        for (int i = 0; i != numLevels; i++) {
            if (i == current) {
                SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
                int collected = scene->getSinglePlayerObject()->getNumCollectablesCollected();
                totalBags += collected;
                if (collected != scene->getMap().getNumCollectablesTotal()) {
                    allFound = false;
                }
            } else {
                const Level* level = levelData->getLevel(i);
                if (!level->isLevelComplete()) {
                    allFound = false;
                    continue;
                }
                int found = level->getNumBagsFound();
                totalBags += found;
                if (level->getNumTotalBags() != found) {
                    allFound = false;
                }
            }
        }
        if (allFound) {
            registerAchievementComplete(ACHIEVEMENT_ALL_BAGS);
        }
    }
    completeReached(this, m_bagAchievements, totalBags);
}

void AchievementData::registerGameEnd() {
    m_gameEndCount++;
    registerAchievementComplete(ACHIEVEMENT_GAME_COMPLETE);
    std::vector<CountAchievement*>::iterator it, end;
    for (it = m_deathAchievements.begin(), end = m_deathAchievements.end(); it != end; ++it) {
        CountAchievement* achievement = *it;
        if (achievement->getCount() < m_totalMetrics.m_deaths) {
            registerAchievementComplete(achievement->getIdx());
        }
    }
    for (it = m_badLandingAchievements.begin(), end = m_badLandingAchievements.end(); it != end; ++it) {
        CountAchievement* achievement = *it;
        if (achievement->getCount() < m_totalMetrics.m_badLandings) {
            registerAchievementComplete(achievement->getIdx());
        }
    }
}

bool AchievementData::registerLevelComplete() {
    bool completed = false;
    LevelAchievement* levelAchievement = getLevelAchievement(m_currentLevel);
    if (levelAchievement != NULL) {
        completed = registerAchievementComplete(levelAchievement->getIdx());
    }
    AppEngine::getCanvas();
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_SPEEDRUN) {
        TimedLevelAchievement* timed = getTimedLevelAchievement(m_currentLevel);
        if (timed != NULL && m_levelMetrics.m_timeMillis < timed->getTime()) {
            registerAchievementComplete(timed->getIdx());
        }
    }
    return completed;
}

void AchievementData::registerSprintOn(int raceTime) {
    std::vector<TimedAchievement*>& list = m_sprintAchievements;
    for (std::vector<TimedAchievement*>::iterator it = list.begin(), end = list.end(); it != end; ++it) {
        (*it)->runTimer(raceTime >> 10);
    }
}

void AchievementData::registerDisarm() {
    m_levelMetrics.m_disarms++;
    completeReached(this, m_disarmAchievements, m_levelMetrics.m_disarms + m_totalMetrics.m_disarms);
    saveRmsData();
}

void AchievementData::registerEnemyFall() {
    m_levelMetrics.m_enemyFalls++;
    completeReached(this, m_enemyFallAchievements, m_levelMetrics.m_enemyFalls + m_totalMetrics.m_enemyFalls);
    saveRmsData();
}

void AchievementData::registerKill(int time) {
    m_levelMetrics.m_kills++;
    completeReached(this, m_killAchievements, m_levelMetrics.m_kills + m_totalMetrics.m_kills);

    std::vector<EventsInPhaseAchievement*>& inPhase = m_killsInAttackAchievements;
    for (std::vector<EventsInPhaseAchievement*>::iterator it = inPhase.begin(), end = inPhase.end(); it != end;
         ++it) {
        (*it)->eventHappended();
    }
    std::vector<TimedEventsAchievement*>& timed = m_timedKillAchievements;
    for (std::vector<TimedEventsAchievement*>::iterator it = timed.begin(), end = timed.end(); it != end; ++it) {
        (*it)->eventHappended(time);
    }
    static_cast<EventsInPhaseAchievement*>(m_achievements[ACHIEVEMENT_ZIPLINE_KILL])->eventHappended();
    saveRmsData();
}

void AchievementData::registerRivalKill(int raceTime) {
    registerKill(raceTime);
    m_levelMetrics.m_rivalKills++;
    completeReached(this, m_rivalKillAchievements, m_levelMetrics.m_rivalKills + m_totalMetrics.m_rivalKills);
    saveRmsData();
}

void AchievementData::registerLevelEnd() {
    m_totalMetrics += m_levelMetrics;
    saveRmsData();
#ifdef MEDECOMP_PORT
    medecomp_achievements_saved();
#endif
    m_currentLevel = 0;
    m_levelRunning = false;
}

AchievementData::~AchievementData() {
    saveRmsData();
}

void AchievementData::loadRmsData() {
    midp::ReferenceCountedPointer<midp::String> name = new midp::String(ACHIEVEMENT_FILENAME);
    midp::ReferenceCountedPointer<midp::InputStream> file = midp::Class::getResourceAsStream(name);
    if (file != NULL) {
        midp::DataInputStream in(file);
        if (in.available() >= 4 && in.readInt() == m_achievements.m_length) {
            m_totalMetrics.read(in);
            if (in.available() >= m_achievements.m_length) {
                for (int i = 0; i < m_achievements.m_length; i++) {
                    if (in.readBoolean()) {
                        m_achievements[i]->complete();
                    }
                }
            }
        }
    }
}

void AchievementData::loadData() {
    if (!(m_achievements == NULL)) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_ACHIEVEMENT_DATA));
    int count = in.readShort();
    m_achievements = midp::array<Achievement*>(count);
    for (int i = 0; i != count; i++) {
        int type = in.readByte();
        int nameId = STRING_ARRAY[in.readShort()];
        int descriptionId = STRING_ARRAY[in.readShort()];
        int value = in.readShort();
        int value2 = in.readShort();

        Achievement* achievement = NULL;
        switch (type) {
        case TYPE_LEVEL:
            achievement = addTo(m_levelAchievements, new LevelAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_TIMED_LEVEL:
            achievement = addTo(m_timedLevelAchievements,
                                new TimedLevelAchievement(i, nameId, descriptionId, value, value2));
            break;
        case TYPE_BAGS_PER_LEVEL:
            achievement = addTo(m_bagsPerLevelAchievements,
                                new BagsPerLevelAchievement(i, nameId, descriptionId, value, value2));
            break;
        case TYPE_BAGS:
            achievement = addTo(m_bagAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_STARS:
            achievement = addTo(m_starAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_DEATHS:
            achievement = addTo(m_deathAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_BAD_LANDINGS:
            achievement = addTo(m_badLandingAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_UNUSED_7:
            achievement = addTo(m_unk74, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_RUN_DIST:
            achievement = addTo(m_runDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_WALL_RUN_DIST:
            achievement = addTo(m_wallRunDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_CLIMB_DIST:
            achievement = addTo(m_climbDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_ZIPLINE_DIST:
            achievement = addTo(m_ziplineDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_SLIDE_DIST:
            achievement = addTo(m_slideDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_BALANCE_DIST:
            achievement = addTo(m_balanceDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_FALL_DIST:
            achievement = addTo(m_fallDistAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_SPRINT:
            achievement = addTo(m_sprintAchievements, new TimedAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_KILLS:
            achievement = addTo(m_killAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_DISARMS:
            achievement = addTo(m_disarmAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_ENEMY_FALLS:
            achievement = addTo(m_enemyFallAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_KILLS_IN_ATTACK:
            achievement = addTo(m_killsInAttackAchievements,
                                new EventsInPhaseAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_TIMED_KILLS:
            achievement = addTo(m_timedKillAchievements,
                                new TimedEventsAchievement(i, nameId, descriptionId, value, value2));
            break;
        case TYPE_RIVAL_KILLS:
            achievement = addTo(m_rivalKillAchievements, new CountAchievement(i, nameId, descriptionId, value));
            break;
        case TYPE_ZIPLINE_KILL:
            achievement = new EventsInPhaseAchievement(i, nameId, descriptionId, 1);
            break;
        case TYPE_POPUP:
            achievement = addTo(m_popupAchievements, new Achievement(i, nameId, descriptionId));
            break;
        default:
            achievement = addTo(m_otherAchievements, new Achievement(i, nameId, descriptionId));
            break;
        }
        m_achievements[i] = achievement;
    }
    in.close();
    loadRmsData();
}

Achievement* AchievementData::getAchievement(int idx) const {
    return m_achievements[idx];
}

int AchievementData::getAchievementNum() {
    return m_achievements.m_length;
}
