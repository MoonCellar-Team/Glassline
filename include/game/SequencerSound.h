#pragma once

#include "game/SequencerEvent.h"

class SequencerSound : public SequencerEvent {
public:
    explicit SequencerSound(midp::DataInputStream& in);
    virtual ~SequencerSound();
    virtual void play(SoundSequencer* sequencer, const GameObject* obj);

    int m_soundId;
};
