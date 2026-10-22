#pragma once

#include "runtime/midp/array.h"

class GameObject;
class SceneGame;
class SequencerTrack;
class SequencerTrackPlayer;
class SoundEventPoolManager;
class SoundManager;

class SoundSequencer {
public:
    static const int NUM_TRACK_PLAYERS = 10;

    SoundSequencer(SceneGame* scene, SoundManager& soundManager, SoundEventPoolManager& pools);
    ~SoundSequencer();
    void update(int dt);
    void loadData();
    int playSequence(int id, const GameObject* obj);
    void stopSequence(int handle);
    bool isSequencePlaying(int handle);
    SoundManager& getSoundManager();
    SoundEventPoolManager& getSoundPoolManager();

    SceneGame* m_scene;
    SoundManager* m_soundManager;
    SoundEventPoolManager* m_pools;
    midp::array<SequencerTrack*> m_tracks;
    SequencerTrackPlayer* m_trackPlayers[NUM_TRACK_PLAYERS];
};
