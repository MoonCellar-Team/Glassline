#include "game/GhostAnimationPlayback.h"

#include "game/GhostAnimation.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;

static const char* const CHALLENGE_GHOST_FILENAME = "ChallengeGhost";

static void readKeyframePose(midp::DataInputStream& in, GhostKeyframe& keyframe) {
    keyframe.m_position.x = (float)in.readInt() * FIXED_TO_FLOAT;
    keyframe.m_position.y = (float)in.readInt() * FIXED_TO_FLOAT;
    keyframe.m_position.z = (float)in.readInt() * FIXED_TO_FLOAT;
    keyframe.m_visual = (short)in.readUnsignedShort();
    keyframe.m_blendWeight = (float)in.readInt() * FIXED_TO_FLOAT;
}

GhostAnimationPlayback::~GhostAnimationPlayback() {}

GhostAnimationPlayback::GhostAnimationPlayback(midp::InputStream* in) {
    midp::DataInputStream data(in);
    int count = data.readInt();
    m_keyframes = midp::array<GhostKeyframe>(count);
    int last = count - 1;
    for (int i = 0; i != last; i++) {
        GhostKeyframe& keyframe = m_keyframes[i];
        readKeyframePose(data, keyframe);
        keyframe.m_deltaTime = data.readUnsignedShort();
    }
    readKeyframePose(data, m_keyframes[last]);
}

GhostAnimationPlayback* GhostAnimationPlayback::createFromChallenge() {
    GhostAnimationPlayback* playback = NULL;
    midp::ReferenceCountedPointer<midp::InputStream> in =
        midp::Class::getResourceAsStream(new midp::String(CHALLENGE_GHOST_FILENAME));
    if (in != NULL && in->read() == GhostAnimation::FILE_VERSION) {
        playback = new GhostAnimationPlayback(in);
    }
    return playback;
}

GhostAnimationPlayback* GhostAnimationPlayback::createForCurrentLevel() {
    GhostAnimationPlayback* playback = NULL;
    midp::ReferenceCountedPointer<midp::InputStream> in =
        midp::Class::getResourceAsStream(new midp::String(GhostAnimation::getCurrentLevelGhostFilename()));
    if (in != NULL && in->read() == GhostAnimation::FILE_VERSION) {
        playback = new GhostAnimationPlayback(in);
    }
    return playback;
}

int GhostAnimationPlayback::getKeyframeNum() {
    return m_keyframes.m_length;
}

GhostKeyframe* GhostAnimationPlayback::getKeyframe(int index) {
    return &m_keyframes[index];
}
