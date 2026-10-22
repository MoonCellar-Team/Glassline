#include "game/SoundEventPoolManager.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "game/SoundManager.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_SOUND_POOLS = 224;

SoundEventPoolManager::SoundEventPoolManager() {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_SOUND_POOLS));
    int poolCount = in.readUnsignedShort();
    m_pools = midp::array<midp::array<midp::array<int> > >(poolCount);
    for (int pool = 0; pool < poolCount; pool++) {
        int groupCount = in.readUnsignedShort();
        m_pools[pool] = midp::array<midp::array<int> >(groupCount);
        for (int group = 0; group < groupCount; group++) {
            int soundCount = in.readUnsignedShort();
            m_pools[pool][group] = midp::array<int>(soundCount);
            for (int i = 0; i < soundCount; i++) {
                int id = ResourceManager::SOUND_EVENT_LOOKUP[in.readUnsignedShort()];
                m_pools[pool][group][i] = id;
                sound.loadEvent(id);
            }
        }
    }
    in.close();
}

int SoundEventPoolManager::getRandomSoundEventID(int pool, int group, int excludeId) {
    int count = m_pools[pool][group].m_length;
    int index = AppEngine::getCanvas()->rand(0, count - 1);
    midp::array<int>& sounds = m_pools[pool][group];
    int id = sounds[index];
    if (id == excludeId) {
        id = sounds[(index + 1) % count];
    }
    return id;
}

void SoundEventPoolManager::unloadSounds() {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    int poolCount = m_pools.m_length;
    for (int pool = 0; pool < poolCount; pool++) {
        midp::array<midp::array<int> >& groups = m_pools[pool];
        int groupCount = groups.m_length;
        groups = midp::array<midp::array<int> >(groupCount);
        for (int group = 0; group < groupCount; group++) {
            midp::array<int>& sounds = m_pools[pool][group];
            int soundCount = sounds.m_length;
            sounds = midp::array<int>(soundCount);
            for (int i = 0; i < soundCount; i++) {
                sound.unloadEvent(m_pools[pool][group][i]);
            }
        }
    }
}

SoundEventPoolManager::~SoundEventPoolManager() {
    unloadSounds();
}
