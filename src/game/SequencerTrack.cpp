#include "game/SequencerTrack.h"

#include "game/AppEngine.h"
#include "game/GameScreen.h"
#include "game/SceneGame.h"
#include "game/SequencerFootstep.h"
#include "game/SequencerPoolSound.h"
#include "game/SequencerSound.h"
#include "runtime/midp/DataInputStream.h"

SequencerTrack::~SequencerTrack() {
    for (int i = 0; i < m_events.m_length; i++) {
        delete m_events[i];
    }
}

SequencerTrack::SequencerTrack(midp::DataInputStream& in) : m_flags(0), m_length(0) {
    int flags = in.readByte();
    int length = in.readInt();

    int footstepCount = (unsigned char)in.readByte();
    midp::array<SequencerFootstep*> footsteps(footstepCount);
    if (footstepCount != 0) {
        SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
        int screen = 0;
        if ((flags & FLAG_SECOND_SCREEN_PLAYER) && scene->numGameScreens() >= 2) {
            screen = 1;
        }
        GameObjectPlayer* player = scene->getGameScreen(screen)->getPlayerObject();
        for (int i = 0; i < footstepCount; i++) {
            footsteps[i] = new SequencerFootstep(in, player);
        }
    }

    int soundCount = (unsigned char)in.readByte();
    midp::array<SequencerSound*> sounds(soundCount);
    for (int i = 0; i < soundCount; i++) {
        sounds[i] = new SequencerSound(in);
    }

    int poolSoundCount = (unsigned char)in.readByte();
    midp::array<SequencerPoolSound*> poolSounds(poolSoundCount);
    for (int i = 0; i < poolSoundCount; i++) {
        poolSounds[i] = new SequencerPoolSound(in);
    }

    static const int NO_EVENT = 0x7fffffff;
    int total = footstepCount + soundCount + poolSoundCount;
    midp::array<SequencerEvent*> events(total);
    int footstep = 0;
    int sound = 0;
    int poolSound = 0;
    for (int i = 0; i < total; i++) {
        int footstepTime = NO_EVENT;
        int soundTime = NO_EVENT;
        int poolSoundTime = NO_EVENT;
        if (footstep < footstepCount) {
            footstepTime = footsteps[footstep]->m_time;
        }
        if (sound < soundCount) {
            soundTime = sounds[sound]->m_time;
        }
        if (poolSound < poolSoundCount) {
            poolSoundTime = poolSounds[poolSound]->m_time;
        }
        if (footstepTime < soundTime && footstepTime < poolSoundTime) {
            events[i] = footsteps[footstep++];
        }
        if (soundTime < footstepTime && soundTime < poolSoundTime) {
            events[i] = sounds[sound++];
        }
        if (poolSoundTime < soundTime && poolSoundTime < footstepTime) {
            events[i] = poolSounds[poolSound++];
        }
    }

    m_flags = (unsigned char)flags;
    m_length = length;
    m_events = events;
}
