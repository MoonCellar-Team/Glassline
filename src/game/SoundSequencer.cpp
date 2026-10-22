#include "game/SoundSequencer.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "game/SequencerTrack.h"
#include "game/SequencerTrackPlayer.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_SOUND_SEQUENCER = 222;

SoundSequencer::SoundSequencer(SceneGame* scene, SoundManager& soundManager, SoundEventPoolManager& pools)
    : m_scene(scene), m_soundManager(&soundManager), m_pools(&pools) {
    for (int i = 0; i < NUM_TRACK_PLAYERS; i++) {
        m_trackPlayers[i] = new SequencerTrackPlayer(this);
    }
}

void SoundSequencer::stopSequence(int handle) {
    if (handle == -1) {
        return;
    }
    m_trackPlayers[handle]->stop();
}

bool SoundSequencer::isSequencePlaying(int handle) {
    if (handle == -1) {
        return false;
    }
    return m_trackPlayers[handle]->isPlaying();
}

int SoundSequencer::playSequence(int id, const GameObject* obj) {
    for (int i = 0; i < NUM_TRACK_PLAYERS; i++) {
        SequencerTrackPlayer* player = m_trackPlayers[i];
        if (!player->isPlaying()) {
            player->play(m_tracks[id], obj);
            return i;
        }
    }
    return -1;
}

void SoundSequencer::update(int dt) {
    for (int i = 0; i < NUM_TRACK_PLAYERS; i++) {
        SequencerTrackPlayer* player = m_trackPlayers[i];
        if (player->isPlaying()) {
            player->update(dt);
        }
    }
}

SoundSequencer::~SoundSequencer() {
    for (int i = 0; i < m_tracks.m_length; i++) {
        delete m_tracks[i];
    }
    for (int i = 0; i < NUM_TRACK_PLAYERS; i++) {
        delete m_trackPlayers[i];
    }
}

void SoundSequencer::loadData() {
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_SOUND_SEQUENCER));
    int count = in.readShort();
    m_tracks = midp::array<SequencerTrack*>(count);
    for (int i = 0; i < count; i++) {
        m_tracks[i] = new SequencerTrack(in);
    }
    in.close();
    for (int i = 0; i < NUM_TRACK_PLAYERS; i++) {
        m_trackPlayers[i]->reset();
    }
}

SoundManager& SoundSequencer::getSoundManager() {
    return *m_soundManager;
}

SoundEventPoolManager& SoundSequencer::getSoundPoolManager() {
    return *m_pools;
}
