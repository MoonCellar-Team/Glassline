#pragma once

class GameObject;
class SequencerTrack;
class SoundSequencer;

class SequencerTrackPlayer {
public:
    explicit SequencerTrackPlayer(SoundSequencer* sequencer);

    void reset();
    void play(SequencerTrack* track, const GameObject* obj);
    void stop();
    bool isPlaying();
    void update(int dt);

    SoundSequencer* m_sequencer;
    SequencerTrack* m_track;
    const GameObject* m_object;
    bool m_playing;
    int m_time;
    int m_nextEvent;
};
