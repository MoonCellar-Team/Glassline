#pragma once

#include "game/GhostKeyframe.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/array.h"

namespace midp {
class InputStream;
}

class GhostAnimationPlayback : public midp::ReferenceCounted {
public:
    explicit GhostAnimationPlayback(midp::InputStream* in);
    virtual ~GhostAnimationPlayback();

    static GhostAnimationPlayback* createFromChallenge();
    static GhostAnimationPlayback* createForCurrentLevel();

    int getKeyframeNum();
    GhostKeyframe* getKeyframe(int index);

    midp::array<GhostKeyframe> m_keyframes;
};
