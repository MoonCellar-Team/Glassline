#pragma once

namespace midp {
class DataInputStream;
}
class GameObject;
class SoundSequencer;

class SequencerEvent {
public:
    explicit SequencerEvent(midp::DataInputStream& in);
    virtual ~SequencerEvent();
    virtual void play(SoundSequencer* sequencer, const GameObject* obj) = 0;

    int m_time;
};
