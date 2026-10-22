#pragma once

#include "game/SequencerEvent.h"

class GameObjectPlayer;

class SequencerFootstep : public SequencerEvent {
public:
    SequencerFootstep(midp::DataInputStream& in, GameObjectPlayer* player);
    virtual ~SequencerFootstep();
    virtual void play(SoundSequencer* sequencer, const GameObject* obj);

    GameObjectPlayer* m_player;
};
