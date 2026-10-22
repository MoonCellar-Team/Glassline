#pragma once

#include "runtime/midp/array.h"

class SoundEventPoolManager {
public:
    SoundEventPoolManager();
    ~SoundEventPoolManager();

    int getRandomSoundEventID(int pool, int group, int excludeId);
    void unloadSounds();

    midp::array<midp::array<midp::array<int> > > m_pools;
};
