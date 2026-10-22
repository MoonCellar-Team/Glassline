#pragma once

#include "game/SequencerEvent.h"

class SequencerPoolSound : public SequencerEvent {
public:
    explicit SequencerPoolSound(midp::DataInputStream& in);
    virtual ~SequencerPoolSound();
    virtual void play(SoundSequencer* sequencer, const GameObject* obj);

    int m_pool;
    int m_group;
};
