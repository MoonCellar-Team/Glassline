#include "game/SequencerEvent.h"

#include "runtime/midp/DataInputStream.h"

SequencerEvent::SequencerEvent(midp::DataInputStream& in) : m_time(0) {
    m_time = in.readInt();
}

SequencerEvent::~SequencerEvent() {}
