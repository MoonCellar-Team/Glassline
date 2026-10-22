#include "game/SequencerTrackPlayer.h"

#include "game/SequencerEvent.h"
#include "game/SequencerTrack.h"

SequencerTrackPlayer::SequencerTrackPlayer(SoundSequencer* sequencer)
    : m_sequencer(sequencer), m_track(NULL), m_object(NULL), m_playing(false), m_time(0), m_nextEvent(-1) {}

void SequencerTrackPlayer::reset() {
    m_track = NULL;
    m_playing = false;
    m_time = 0;
    m_nextEvent = -1;
}

void SequencerTrackPlayer::play(SequencerTrack* track, const GameObject* obj) {
    m_track = track;
    m_playing = true;
    m_time = 0;
    m_nextEvent = 0;
    m_object = obj;
}

void SequencerTrackPlayer::stop() {
    reset();
}

bool SequencerTrackPlayer::isPlaying() {
    return m_playing;
}

void SequencerTrackPlayer::update(int dt) {
    if (!m_playing) {
        return;
    }
    m_time += dt;

    while (m_nextEvent < m_track->m_events.m_length) {
        SequencerEvent* event = m_track->m_events[m_nextEvent];
        if (event->m_time > m_time) {
            break;
        }
        event->play(m_sequencer, m_object);
        m_nextEvent++;
    }

    int length = m_track->m_length;
    if (m_time < length) {
        return;
    }
    if (m_track->m_flags & SequencerTrack::FLAG_LOOP) {
        m_time -= length;
        m_nextEvent = 0;
    } else {
        reset();
    }
}
