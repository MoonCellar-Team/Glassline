#include "game/LevelData.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/IFFReader.h"
#include "game/IFFWriter.h"
#include "game/ResourceManager.h"
#include "game/TextManager.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#include "runtime/util/VectorSE.h"

#ifdef MEDECOMP_PORT
extern "C" int medecomp_player_level_playing(void);
extern "C" int medecomp_player_level_record(int timeMs);
#endif

static const int RESOURCE_LEVEL_DATA = 200;

static const char* const SAVE_NAME = "LevelProgress";
static const int SAVE_VERSION = 2;

static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;

const short LevelData::STRING_ARRAY[] = {
    2048, 2097, 2098, 2228, 2240, 2099, 2229, 2241, 2100, 2230, 2242, 2101,
    2231, 2243, 2102, 2232, 2244, 2103, 2233, 2245, 2104, 2234, 2246, 2105,
    2235, 2247, 2106, 2236, 2248, 2107, 2237, 2249, 2108, 2238, 2250, 2109,
    2239, 2251, 2110, 2353, 2354, 2355, 2356,    0,    0,    0,    0,    0,
};

const int LevelData::RESOURCE_ARRAY[] = {
    202, 130, 129, 131,  -1,   6,   0, 145, 225, 135,   7,   1,
    226,   4,   2, 208, 185,  20,   3, 209, 186, 136,  31,   4,
    210, 187, 137,   9,   5, 211, 188, 139,   6, 212, 189,  27,
      7, 213, 190, 138,  33,   8, 214, 191,   1,   9, 215, 192,
     32,  10, 216, 193,  11, 217, 194,  24,  12, 218, 195,   0,
     13, 219, 196, 204, 205, 206, 207,
};

const short LevelData::RUNNER_ANIM_ARRAY[] = {45, 44, 40, 41, 42, 43, 0, 0, 0, 0};

const short LevelData::QUAD_ARRAY[] = {
     6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 37, 35, 36, 34,  0,  0,  0,  0,  0,  0,
};

const short LevelData::MODEL_ARRAY[] = {
    11, 7, 12, 8, 13, 9, 14, 10, 15, 11, 16, 12, 17, 18, 19, 20,
};

static bool isStoryType(int type) {
    return (unsigned)type < 2;
}

const Level* LevelData::getLevel(int index) const {
    return &m_levels[index];
}

int LevelData::getGameMode() const {
    return m_gameMode;
}

bool LevelData::isRaceMode() const {
    return (unsigned)(m_gameMode - GAME_MODE_SPEEDRUN) < 3;
}

bool LevelData::isMultiplayerMode() const {
    return (unsigned)(m_gameMode - GAME_MODE_MULTIPLAYER_RACE) < 2;
}

int LevelData::getLevelTimeLimitMillis() const {
    return m_levelTimeLimitMillis;
}

bool LevelData::isMultiplayerCombatAllowed() const {
    if (isMultiplayerMode()) {
        return m_multiplayerCombatAllowed;
    }
    return false;
}

int LevelData::getCurrentLevelIndex() const {
    return m_currentLevel;
}

const Level* LevelData::getCurrentLevelObject() const {
    return &m_levels[m_currentLevel];
}

bool LevelData::isCurrentLevelLastStory() {
    return getNextStoryLevel(m_currentLevel) == -1;
}

int LevelData::getNumUnlockedLevels() {
    return m_numUnlockedLevels;
}

const ModelSet* LevelData::getModelSet(int index) const {
    return &m_modelSets[index];
}

void LevelData::setCurrentLevelByIndex(GameMode mode, int index) {
    m_gameMode = mode;
    m_currentLevel = index;
}

bool LevelData::isGameNewlyCompleted() {
    return m_gameNewlyCompleted;
}

void LevelData::setGameNotNewlyCompleted() {
    m_gameNewlyCompleted = false;
}

int LevelData::calculateNumCollectedStars() const {
    int stars = 0;
    int count = m_levels.m_length;
    for (int i = 0; i != count; i++) {
        stars += m_levels[i].getNumStarsAchieved();
    }
    return stars;
}

int LevelData::getCurrentLoadingScreen() {
    if (m_gameMode == GAME_MODE_MULTIPLAYER_RACE) {
        return LOADING_SCREEN_MULTIPLAYER_RACE;
    }
    if (m_gameMode == GAME_MODE_MULTIPLAYER_TIMED) {
        return LOADING_SCREEN_MULTIPLAYER_TIMED;
    }
    return m_levels[m_currentLevel].getLoadingScreen();
}

bool LevelData::isStoryLevelComplete() {
    int count = m_levels.m_length;
    for (int i = 0; i != count; i++) {
        Level& level = m_levels[i];
        if (isStoryType(level.getType()) && level.isLevelComplete()) {
            return true;
        }
    }
    return false;
}

void LevelData::nextStoryLevel() {
    int count = m_levels.m_length;
    for (int i = m_currentLevel + 1; i != count; i++) {
        if (isStoryType(m_levels[i].getType())) {
            m_currentLevel = i;
            return;
        }
    }
}

int LevelData::getNextStoryLevel(int index) {
    int count = m_levels.m_length;
    while (++index < count) {
        if (isStoryType(m_levels[index].getType())) {
            return index;
        }
    }
    return -1;
}

int LevelData::calcNextContinueLevel() {
    int unlocked = m_numUnlockedLevels;
    if (unlocked == 0) {
        return -1;
    }
    if (getNextStoryLevel(unlocked - 1) != -1) {
        return unlocked;
    }
    if (m_lastCompletedStoryLevel == -1) {
        return -1;
    }
    return getNextStoryLevel(m_lastCompletedStoryLevel);
}

void LevelData::setCurrentLevelByUID(GameMode mode, const midp::String* uid) {
    m_gameMode = mode;
    for (int i = 0; i != m_levels.m_length; i++) {
        if (m_levels[i].getUniqueName()->equals(uid)) {
            m_currentLevel = i;
            return;
        }
    }
}

LevelData::LevelData()
    : m_numUnlockedLevels(0),
      m_gameMode(GAME_MODE_STORY),
      m_currentLevel(0),
      m_lastCompletedStoryLevel(-1),
      m_gameNewlyCompleted(false),
      m_levelTimeLimitMillis(0),
      m_multiplayerCombatAllowed(false) {}

void LevelData::resetRmsData() {
    m_numUnlockedLevels = 0;
    for (int i = 0; i != m_levels.m_length; i++) {
        m_levels[i].resetRms(false);
    }
}

LevelData::~LevelData() {}

void LevelData::saveRmsData() {
    if (AppEngine::getCanvas()->storageFull()) {
        return;
    }
    IFFWriter writer(midp::OutputStream::getResourceAsStream(new midp::String(SAVE_NAME)));
    writer.writeChunk("VERS")->writeInt(SAVE_VERSION);

    midp::DataOutputStream* out = writer.writeChunk("PROG");
    out->writeShort((short)m_numUnlockedLevels);
    out->writeShort((short)m_lastCompletedStoryLevel);

    for (int i = 0; i < m_levels.m_length; i++) {
        out = writer.writeChunk("LEVL");
        out->writeShort((short)i);
        m_levels[i].writeRms(out);
    }
}

bool LevelData::setLevelComplete(int timeMillis, util::VectorSE<GameObject>& bags) {
#ifdef MEDECOMP_PORT
    if (medecomp_player_level_playing()) return medecomp_player_level_record(timeMillis) != 0;
#endif
    Level& level = m_levels[m_currentLevel];
    if (m_gameMode == GAME_MODE_STORY && isCurrentLevelLastStory() && !level.m_complete) {
        m_gameNewlyCompleted = true;
    }
    level.m_complete = true;

    bool newRecord = false;
    if (m_gameMode == GAME_MODE_SPEEDRUN) {
        if (level.m_bestSpeedRunTimeMillis == -1 || timeMillis < level.m_bestSpeedRunTimeMillis) {
            level.m_bestSpeedRunTimeMillis = timeMillis;
            AppEngine::getAchievementData()->registerStars();
            newRecord = true;
        }
    } else if (m_gameMode == GAME_MODE_STORY) {
        int count = bags.size();
        if (level.m_bagsFound.m_length == count) {
            for (int i = 0; i != count; i++) {
                bool& found = level.m_bagsFound[i];
                found = found || bags.elementAt(i) == NULL;
            }
        } else {
            level.m_bagsFound = midp::array<bool>(count);
            for (int i = 0; i != count; i++) {
                level.m_bagsFound[i] = bags.elementAt(i) == NULL;
            }
        }
        m_numUnlockedLevels = midp::Math::max(m_numUnlockedLevels, m_currentLevel + 1);
        m_lastCompletedStoryLevel = m_currentLevel;
    }
    saveRmsData();
    return newRecord;
}

void LevelData::resetLevelData() {
    m_numUnlockedLevels = 0;
    m_lastCompletedStoryLevel = -1;
    for (int i = 0; i != m_levels.m_length; i++) {
        m_levels[i].levelReset();
    }
    saveRmsData();
}

void LevelData::loadRmsData() {
    resetRmsData();
    midp::InputStream* file = midp::Class::getResourceAsStream(new midp::String(SAVE_NAME));
    if (file == NULL) {
        return;
    }
    IFFReader reader(file);
    while (!reader.isReadComplete()) {
        if (reader.isIdOfCurrectChunk("VERS")) {
            int version;
            {
                midp::DataInputStream in(reader.readChunk());
                version = in.readInt();
            }
            if (version != SAVE_VERSION) {
                break;
            }
        } else if (reader.isIdOfCurrectChunk("PROG")) {
            midp::InputStream* chunk = reader.readChunk();
            midp::DataInputStream in(chunk);
            if (chunk->available() >= 2) {
                int unlocked = in.readShort();
                if (unlocked >= 0 && unlocked <= m_levels.m_length) {
                    m_numUnlockedLevels = unlocked;
                }
            }
            if (chunk->available() >= 2) {
                m_lastCompletedStoryLevel = in.readShort();
            }
        } else if (reader.isIdOfCurrectChunk("LEVL")) {
            midp::InputStream* chunk = reader.readChunk();
            if (chunk->available() >= 2) {
                midp::DataInputStream in(chunk);
                int index = in.readShort();
                if (index >= 0 && index < m_levels.m_length) {
                    if (!m_levels[index].readRms(in)) {
                        m_levels[index].resetRms(true);
                    }
                }
            }
        } else {
            reader.skipChunk();
        }
    }
}

void LevelData::loadData() {
    if (!(m_levels == NULL)) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_LEVEL_DATA));
    midp::StringBuffer* name = AppEngine::getCanvas()->getTextManager()->clearStringBuffer();

    int count = in.readShort();
    m_levels = midp::array<Level>(count);
    for (int i = 0; i < count; i++) {
        Level& level = m_levels[i];

        name->setLength(0);
        for (int n = in.readByte(); n != 0; n--) {
            name->append((char)in.readByte());
        }
        level.m_uniqueName = name->toString();

        level.m_type = in.readByte();
        level.m_name = STRING_ARRAY[in.readShort()];
        level.m_objective = STRING_ARRAY[in.readShort()];
        level.m_dateTime = STRING_ARRAY[in.readShort()];
        level.m_mapResId = RESOURCE_ARRAY[in.readShort()];
        level.m_mapResIdMultiplayer = RESOURCE_ARRAY[in.readShort()];
        for (int s = 0; s < Level::NUM_STAR_TIMES; s++) {
            int minutes = in.readByte();
            int seconds = in.readByte();
            level.m_speedRunRequirementMillis[s] = (short)(minutes * 60 + seconds) * 1000;
        }
        level.m_loadingScreen = RESOURCE_ARRAY[in.readShort()];
        level.m_startMusicId = RESOURCE_ARRAY[in.readShort()];
        level.m_introFaithAnim = RUNNER_ANIM_ARRAY[in.readShort()];
        level.m_introCamAnim = RESOURCE_ARRAY[in.readShort()];
        for (int p = 0; p < 2; p++) {
            level.m_introFaithAnimMultiplayer[p] = RUNNER_ANIM_ARRAY[in.readShort()];
            level.m_introCamAnimMultiplayer[p] = RESOURCE_ARRAY[in.readShort()];
        }
        level.m_completionBackground = QUAD_ARRAY[in.readShort()];
    }

    count = in.readShort();
    m_modelSets = midp::array<ModelSet>(count);
    for (int i = 0; i < count; i++) {
        ModelSet& set = m_modelSets[i];
        for (int j = 0; j < ModelSet::NUM_MODELS; j++) {
            set.m_modelIds[j] = MODEL_ARRAY[in.readShort()];
        }
        for (int j = 0; j < ModelSet::NUM_MODELS; j++) {
            set.m_userIds[j] = RESOURCE_ARRAY[in.readShort()];
        }
        set.m_rotateY = in.readInt() * FIXED_TO_FLOAT;
        set.m_rotateZ = in.readInt() * FIXED_TO_FLOAT;
        set.m_backgroundFillColor = in.readInt();
    }

    in.close();
    loadRmsData();
}

int LevelData::getLevelNum() const {
    return m_levels.m_length;
}

void LevelData::setLevelTimeLimitMillis(int millis) {
    m_levelTimeLimitMillis = millis;
}

void LevelData::setMultiplayerCombatAllowed(bool allowed) {
    m_multiplayerCombatAllowed = allowed;
}
