#include "game/Level.h"

#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/String.h"

static const int STRING_ID_DEFAULT = 2048;

int Level::getSpeedRunRequirementMillis(int stars) const {
    return m_speedRunRequirementMillis[stars - 1];
}

int Level::getObjective() const {
    return m_objective;
}

int Level::getDateTime() const {
    return m_dateTime;
}

int Level::getStartMusicId() const {
    return m_startMusicId;
}

int Level::getCompletionBackground() const {
    return m_completionBackground;
}

bool Level::isLevelComplete() const {
    return m_complete;
}

int Level::getBestSpeedRunTimeMillis() const {
    return m_bestSpeedRunTimeMillis;
}

int Level::getMinStarsWithTime(int timeMillis) const {
    if (timeMillis == -1) {
        return 0;
    }
    for (int stars = NUM_STAR_TIMES; stars > 0; stars--) {
        if (m_speedRunRequirementMillis[stars - 1] >= timeMillis) {
            return stars;
        }
    }
    return 0;
}

bool Level::isBagFound(int index) const {
    if (index < 0 || index >= m_bagsFound.m_length) {
        return false;
    }
    return m_bagsFound[index];
}

int Level::getNumBagsFound() const {
    int found = 0;
    int count = m_bagsFound.m_length;
    for (int i = 0; i != count; i++) {
        if (m_bagsFound[i]) {
            found++;
        }
    }
    return found;
}

Level::~Level() {}

void Level::levelReset() {
    if (m_bagsFound != NULL) {
        m_bagsFound = midp::array<bool>();
    }
}

void Level::resetRms(bool resetComplete) {
    if (m_bagsFound != NULL) {
        m_bagsFound = midp::array<bool>();
    }
    m_bestSpeedRunTimeMillis = -1;
    if (resetComplete) {
        m_complete = false;
    }
}

Level::Level()
    : m_uniqueName(NULL),
      m_type(0),
      m_name(STRING_ID_DEFAULT),
      m_objective(STRING_ID_DEFAULT),
      m_dateTime(STRING_ID_DEFAULT),
      m_mapResId(-1),
      m_mapResIdMultiplayer(-1),
      m_loadingScreen(0),
      m_startMusicId(0),
      m_introFaithAnim(0),
      m_introCamAnim(-1),
      m_completionBackground(-1),
      m_complete(false),
      m_bestSpeedRunTimeMillis(-1) {}

Level::Level(const Level& other)
    : m_uniqueName(other.m_uniqueName),
      m_type(other.m_type),
      m_name(other.m_name),
      m_objective(other.m_objective),
      m_dateTime(other.m_dateTime),
      m_mapResId(other.m_mapResId),
      m_mapResIdMultiplayer(other.m_mapResIdMultiplayer),
      m_loadingScreen(other.m_loadingScreen),
      m_startMusicId(other.m_startMusicId),
      m_introFaithAnim(other.m_introFaithAnim),
      m_introCamAnim(other.m_introCamAnim),
      m_completionBackground(-1),
      m_complete(other.m_complete),
      m_bestSpeedRunTimeMillis(other.m_bestSpeedRunTimeMillis),
      m_bagsFound(other.m_bagsFound) {
    for (int i = 0; i < 2; i++) {
        m_introFaithAnimMultiplayer[i] = other.m_introFaithAnimMultiplayer[i];
        m_introCamAnimMultiplayer[i] = other.m_introCamAnimMultiplayer[i];
    }
}

int Level::getIntroCamAnim(int playerIndex) const {
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        return m_introCamAnimMultiplayer[playerIndex];
    }
    return m_introCamAnim;
}

int Level::getIntroFaithAnim(int playerIndex) const {
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        return m_introFaithAnimMultiplayer[playerIndex];
    }
    return m_introFaithAnim;
}

int Level::getMapResId() const {
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        return m_mapResIdMultiplayer;
    }
    return m_mapResId;
}

void Level::writeRms(midp::DataOutputStream* out) {
    out->writeBoolean(m_complete);
    out->writeInt(m_bestSpeedRunTimeMillis);
    int count = m_bagsFound.m_length;
    out->writeByte((unsigned char)count);
    for (int i = 0; i != count; i++) {
        out->writeBoolean(m_bagsFound[i]);
    }
}

bool Level::readRms(midp::DataInputStream& in) {
    if (in.available() < 6) {
        return false;
    }
    m_complete = in.readBoolean();
    m_bestSpeedRunTimeMillis = in.readInt();
    int count = in.readByte();
    if (count < 0 || count > MAX_BAGS) {
        return false;
    }
    if (in.available() < count) {
        return false;
    }
    m_bagsFound = midp::array<bool>(count);
    for (int i = 0; i != count; i++) {
        m_bagsFound[i] = in.readBoolean();
    }
    return true;
}

int Level::getType() const {
    return m_type;
}

int Level::getLoadingScreen() const {
    return m_loadingScreen;
}

int Level::getNumStarsAchieved() const {
    return getMinStarsWithTime(getBestSpeedRunTimeMillis());
}

midp::String* Level::getUniqueName() const {
    return m_uniqueName;
}

int Level::getName() const {
    return m_name;
}

int Level::getNumTotalBags() const {
    return m_bagsFound.m_length;
}
