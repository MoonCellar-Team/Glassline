#pragma once

#include "runtime/midp/array.h"

namespace midp {
class DataInputStream;
}
class SequencerEvent;

class SequencerTrack {
public:
    static const int FLAG_LOOP = 1;
    static const int FLAG_SECOND_SCREEN_PLAYER = 4;

    explicit SequencerTrack(midp::DataInputStream& in);
    ~SequencerTrack();

    int m_flags;
    int m_length;
    midp::array<SequencerEvent*> m_events;
};
