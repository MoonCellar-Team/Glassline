#include "game/SequencerPoolSound.h"

#include "game/GameObject.h"
#include "game/ResourceManager.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/SoundSequencer.h"
#include "runtime/midp/DataInputStream.h"

void SequencerPoolSound::play(SoundSequencer* sequencer, const GameObject* obj) {
    int id = sequencer->getSoundPoolManager().getRandomSoundEventID(m_pool, m_group, -1);
    SoundManager& sound = sequencer->getSoundManager();
    if (obj != NULL) {
        sound.playEventAt(id, obj->m_position.x, obj->m_position.y, obj->m_position.z, 1.0f);
    } else {
        sound.playEvent(id, 1.0f);
    }
}

SequencerPoolSound::SequencerPoolSound(midp::DataInputStream& in) : SequencerEvent(in), m_pool(0), m_group(0) {
    m_pool = ResourceManager::SOUND_TRACK_POOL_LOOKUPS[in.readShort()];
    m_group = ResourceManager::SOUND_TRACK_POOL_LOOKUPS[in.readShort()];
}

SequencerPoolSound::~SequencerPoolSound() {}
