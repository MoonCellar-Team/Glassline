#include "game/SequencerFootstep.h"

#include "game/GameObject.h"
#include "game/GameObjectPlayer.h"
#include "game/ResourceManager.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/SoundSequencer.h"

static const int SOUND_POOL_FOOTSTEP = 0;

void SequencerFootstep::play(SoundSequencer* sequencer, const GameObject* obj) {
    int group = ResourceManager::MATERIAL_LOOKUP[m_player->getCurrentMaterial()];
    int id = sequencer->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_FOOTSTEP, group, -1);
    SoundManager& sound = sequencer->getSoundManager();
    if (obj != NULL) {
        sound.playEventAt(id, obj->m_position.x, obj->m_position.y, obj->m_position.z, 1.0f);
    } else {
        sound.playEvent(id, 1.0f);
    }
}

SequencerFootstep::SequencerFootstep(midp::DataInputStream& in, GameObjectPlayer* player)
    : SequencerEvent(in), m_player(player) {}

SequencerFootstep::~SequencerFootstep() {}
