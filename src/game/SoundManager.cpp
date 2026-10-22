#include "game/SoundManager.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Manager.h"
#include "runtime/midp/PitchControl.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/PlayerIPhoneOpenAL.h"
#include "runtime/midp/PositionControl.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"
#include "runtime/midp/VolumeControl.h"

static const int RESOURCE_SOUND_EVENTS = 223;

const float SoundManager::MAX_VOLUME = 1.0f;
const float SoundManager::MIN_VOLUME = 0.0f;

float SoundManager::getVolumeMusic() const {
    return m_volumeMusic;
}

float SoundManager::getVolumeSFX() const {
    return m_volumeSFX;
}

int SoundManager::getHandle(int id, int instance) {
    return id | instance << 16;
}

int SoundManager::getHandleEventId(int handle) {
    return handle & 0xffff;
}

int SoundManager::getHandleInstanceId(int handle) {
    return (unsigned int)handle >> 16;
}

void SoundManager::pause() {}

bool SoundManager::isEventLoaded(int id) {
    return m_players[id].m_length > 0;
}

void SoundManager::setGroupPitch(int group, float pitch) {
    for (int i = 0; i < m_players.m_length; i++) {
        if (m_eventGroups[i] != group) {
            continue;
        }
        for (int j = 0; j < m_players[i].m_length; j++) {
            midp::PitchControl* control = m_players[i][j]->getPitchControl();
            if (control != NULL) {
                control->setPitchFactor(pitch);
            }
        }
    }
}

void SoundManager::setEventPosition(int handle, float x, float y, float z) {
    int id = getHandleEventId(handle);
    int instance = getHandleInstanceId(handle);
    midp::PositionControl* control = m_players[id][instance]->getPositionControl();
    if (control != NULL) {
        control->setPosition(x, y, z);
    }
}

void SoundManager::setLoopCountEvent(int handle, int count) {
    if (handle < 0) {
        return;
    }
    int id = getHandleEventId(handle);
    int instance = getHandleInstanceId(handle);
    midp::Player* player = m_players[id][instance];
    if (player != NULL) {
        player->setLoopCount(count);
    }
}

bool SoundManager::isHandlePlaying(int handle) {
    if (handle >= 0) {
        int id = getHandleEventId(handle);
        int instance = getHandleInstanceId(handle);
        midp::Player* player = m_players[id][instance];
        if (player != NULL) {
            return player->getState() == midp::Player::STARTED;
        }
    }
    return false;
}

int SoundManager::calcEventVolume(int id, int instance) {
    float volume = m_volumeGlobal * m_instanceVolumes[id][instance];
    int group = m_eventGroups[id];
    int flags = m_groupFlags[group];
    if (flags & GROUP_SFX) {
        volume = m_volumeSFX * volume;
    }
    if (flags & GROUP_MUSIC) {
        volume = m_volumeMusic * volume;
    }
    return (int)(m_groupVolumes[group] * volume * 100.0f);
}

void SoundManager::applyVolume(int id, int instance) {
    midp::Player* player = m_players[id][instance];
    if (player == NULL) {
        return;
    }
    midp::VolumeControl* control = player->getVolumeControl();
    if (control == NULL) {
        return;
    }
    control->setVolume(calcEventVolume(id, instance));
}

void SoundManager::setVolumeGroup(int group, float volume) {
    m_groupVolumes[group] = volume;
    for (int i = 0; i < m_players.m_length; i++) {
        if (m_eventGroups[i] != group) {
            continue;
        }
        for (int j = 0; j < m_players[i].m_length; j++) {
            applyVolume(i, j);
        }
    }
}

void SoundManager::setVolumeEvent(int handle, float volume) {
    if (handle < 0) {
        return;
    }
    applyVolume(getHandleEventId(handle), getHandleInstanceId(handle));
}

void SoundManager::stopEvent(int handle) {
    if (handle < 0) {
        return;
    }
    int id = getHandleEventId(handle);
    int instance = getHandleInstanceId(handle);
    midp::Player* player = m_players[id][instance];
    if (player != NULL) {
        player->stop();
        setVolumeEvent(handle, 1.0f);
        player->setMediaTime(0);
    }
}

int SoundManager::playEvent(int id, float volume) {
    midp::array<midp::Player*>& players = m_players[id];
    int count = players.m_length;
    for (int i = 0; i < count; i++) {
        midp::Player* player = players[i];
        if (player->getState() == midp::Player::STARTED) {
            continue;
        }
        int handle = getHandle(id, i);
        player->prefetch();
        player->setLoopCount(0);
        setVolumeEvent(handle, volume);
        midp::PositionControl* position = player->getPositionControl();
        if (position != NULL) {
            if (m_eventFlags[id] & EVENT_2D) {
                position->setSourceRelative(true);
                position->setPosition(0.0f, 0.0f, 0.0f);
            } else {
                position->setSourceRelative(false);
                position->setReferenceDistance(m_eventRefDistances[id]);
            }
        }
        player->start();
        return handle;
    }
    return -1;
}

int SoundManager::playEventAt(int id, float x, float y, float z, float volume) {
    int handle = playEvent(id, volume);
    if (handle != -1) {
        setEventPosition(handle, x, y, z);
    }
    return handle;
}

void SoundManager::setVolumeSFX(float volume) {
    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    } else if (!(volume >= MIN_VOLUME)) {
        volume = MIN_VOLUME;
    }
    m_volumeSFX = volume;
    for (int i = 0; i < m_players.m_length; i++) {
        if (!(m_groupFlags[m_eventGroups[i]] & GROUP_SFX)) {
            continue;
        }
        for (int j = 0; j < m_players[i].m_length; j++) {
            applyVolume(i, j);
        }
    }
}

void SoundManager::setVolumeMusic(float volume) {
    m_volumeMusic = volume;
    for (int i = 0; i < m_players.m_length; i++) {
        if (!(m_groupFlags[m_eventGroups[i]] & GROUP_MUSIC)) {
            continue;
        }
        for (int j = 0; j < m_players[i].m_length; j++) {
            applyVolume(i, j);
        }
    }
}

void SoundManager::setVolumeGlobal(float volume) {
    m_volumeGlobal = volume;
    for (int i = 0; i < m_players.m_length; i++) {
        for (int j = 0; j < m_players[i].m_length; j++) {
            applyVolume(i, j);
        }
    }
}

void SoundManager::resume() {
    midp::PlayerIPhoneOpenAL::createShared();
}

void SoundManager::setListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ) {
    midp::Manager::setListenerOrientation(atX, atY, atZ, upX, upY, upZ);
}

void SoundManager::setListenerVelocity(float x, float y, float z) {
    midp::Manager::setListenerVelocity(x, y, z);
}

void SoundManager::setListenerPosition(float x, float y, float z) {
    midp::Manager::setListenerPosition(x, y, z);
}

SoundManager::SoundManager() : m_volumeGlobal(1.0f), m_volumeMusic(1.0f), m_volumeSFX(1.0f) {}

SoundManager::~SoundManager() {
    int count = m_eventResources.m_length;
    for (int i = 0; i < count; i++) {
        unloadEvent(i);
    }
}

void SoundManager::addPlayerListener(int id, midp::PlayerListener* listener) {
    midp::array<midp::Player*> players = m_players[id];
    for (int i = 0; i < players.m_length; i++) {
        players[i]->addPlayerListener(listener);
    }
}

void SoundManager::unloadEvent(int id) {
    if (id < 0) {
        return;
    }
    midp::array<midp::Player*> players = m_players[id];
    for (int i = 0; i < players.m_length; i++) {
        midp::Player* player = players[i];
        if (player == NULL) {
            continue;
        }
        if (player->getState() == midp::Player::STARTED) {
            player->stop();
            player->setMediaTime(0);
            midp::Thread::sleep(500);
        }
        player->close();
        midp::DECREF(player);
    }
    m_players[id] = midp::array<midp::Player*>();
    m_instanceVolumes[id] = midp::array<float>();
}

midp::Player* SoundManager::loadPlayer(int id) {
    midp::String* name = new midp::String(ResourceManager::RESOURCE_FILENAMES_LIST[m_eventResources[id]]);
    midp::INCREF(name);
    midp::Player* player = midp::Manager::createPlayer(name);
    if (player != NULL) {
        player->realize();
    }
    midp::DECREF(name);
    return player;
}

void SoundManager::loadEvent(int id) {
    int count = m_eventInstances[id];
    midp::array<midp::Player*>& loaded = m_players[id];
    if (loaded.m_length == count) {
        int i = 0;
        while (i < count && loaded[i] != NULL && loaded[i]->getState() != midp::Player::CLOSED) {
            i++;
        }
        if (i >= count) {
            return;
        }
    }

    unloadEvent(id);
    midp::array<midp::Player*> players(count);
    midp::array<float> volumes(count);
    m_players[id] = players;
    m_instanceVolumes[id] = volumes;
    midp::Player* first = NULL;
    for (int i = 0; i < count; i++) {
        midp::Player* player;
        if (i == 0) {
            player = loadPlayer(id);
            first = player;
        } else {
            player = midp::Manager::createDuplicatePlayer(first);
        }
        players[i] = player;
        volumes[i] = 1.0f;
        applyVolume(id, i);
    }
}

void SoundManager::loadData() {
    AppEngine::getCanvas();
    midp::DataInputStream* in = new midp::DataInputStream(ResourceManager::loadBinaryFile(RESOURCE_SOUND_EVENTS));
    midp::INCREF(in);

    int groupCount = in->readByte();
    midp::array<unsigned char> groupFlags(groupCount);
    for (int i = 0; i < groupCount; i++) {
        groupFlags[i] = in->readByte();
    }

    int eventCount = in->readShort();
    midp::array<short> resources(eventCount);
    midp::array<unsigned char> groups(eventCount);
    midp::array<unsigned char> instances(eventCount);
    midp::array<unsigned char> flags(eventCount);
    midp::array<float> refDistances(eventCount);
    for (int i = 0; i < eventCount; i++) {
        resources[i] = ResourceManager::SOUND_DATA_SETS[in->readShort()];
        groups[i] = in->readByte();
        instances[i] = in->readByte();
        flags[i] = in->readByte();
        refDistances[i] = in->readFloat();
    }
    midp::DECREF(in);

    m_groupFlags = groupFlags;
    m_eventResources = resources;
    m_eventGroups = groups;
    m_eventInstances = instances;
    m_eventFlags = flags;
    m_eventRefDistances = refDistances;
    m_players = midp::array<midp::array<midp::Player*> >(eventCount);
    m_instanceVolumes = midp::array<midp::array<float> >(eventCount);
    m_groupVolumes = midp::array<float>(groupCount);
    for (int i = 0; i < groupCount; i++) {
        m_groupVolumes[i] = 1.0f;
    }
}
