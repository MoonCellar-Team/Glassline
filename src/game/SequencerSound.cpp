#include "game/SequencerSound.h"

#include "game/GameObject.h"
#include "game/ResourceManager.h"
#include "game/SoundManager.h"
#include "game/SoundSequencer.h"
#include "runtime/midp/DataInputStream.h"

void SequencerSound::play(SoundSequencer* sequencer, const GameObject* obj) {
    SoundManager& sound = sequencer->getSoundManager();
    if (obj != NULL) {
        sound.playEventAt(m_soundId, obj->m_position.x, obj->m_position.y, obj->m_position.z, 1.0f);
    } else {
        sound.playEvent(m_soundId, 1.0f);
    }
}

SequencerSound::SequencerSound(midp::DataInputStream& in) : SequencerEvent(in), m_soundId(0) {
    m_soundId = ResourceManager::SOUND_TRACK_SOUNDS[in.readShort()];
}

SequencerSound::~SequencerSound() {}
