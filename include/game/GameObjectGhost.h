#pragma once

#include "game/GameObjectRunner.h"

class GhostAnimationPlayback;

class GameObjectGhost : public GameObjectRunner {
public:
    static const int FADE_TIME = 1000;

    GameObjectGhost(MEdgeMap* map, GhostAnimationPlayback* playback);
    virtual ~GameObjectGhost();

    virtual void checkpointActivated();
    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void update(int dt);

    midp::ReferenceCountedPointer<GhostAnimationPlayback> m_playback;
    int m_keyframeIndex;
    int m_keyframeTime;
    bool m_separated;
    int m_fadeInTime;
    int m_time;
    int m_duration;
};
