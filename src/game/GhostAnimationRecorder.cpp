#include "game/GhostAnimationRecorder.h"

#include "game/AppEngine.h"
#include "game/GhostAnimation.h"
#include "game/GhostKeyframe.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/String.h"

static const float FLOAT_TO_FIXED = 65536.0f;

static void writeKeyframePose(midp::DataOutputStream& out, const GhostKeyframe* keyframe) {
    out.writeInt((int)(keyframe->m_position.x * FLOAT_TO_FIXED));
    out.writeInt((int)(keyframe->m_position.y * FLOAT_TO_FIXED));
    out.writeInt((int)(keyframe->m_position.z * FLOAT_TO_FIXED));
    out.writeUnsignedShort(keyframe->m_visual);
    out.writeInt((int)(keyframe->m_blendWeight * FLOAT_TO_FIXED));
}

void GhostAnimationRecorder::reset() {
    m_keyframes.removeAllElements();
}

void GhostAnimationRecorder::addKeyframe(GhostKeyframe* keyframe) {
    m_keyframes.addElement(keyframe);
}

void GhostAnimationRecorder::write(midp::ReferenceCountedPointer<midp::String> filename) {
    if (AppEngine::getCanvas()->storageFull()) {
        return;
    }
    if (filename == NULL) {
        filename = new midp::String(GhostAnimation::getCurrentLevelGhostFilename());
    }
    midp::ReferenceCountedPointer<midp::OutputStream> stream = midp::OutputStream::getResourceAsStream(filename);
    if (stream == NULL) {
        return;
    }

    midp::DataOutputStream out(stream);
    out.writeByte(GhostAnimation::FILE_VERSION);
    int count = m_keyframes.size();
    out.writeInt(count);
    writeKeyframePose(out, m_keyframes.elementAt(0));
    for (int i = 1; i != count; i++) {
        GhostKeyframe* keyframe = m_keyframes.elementAt(i);
        out.writeUnsignedShort(keyframe->m_deltaTime);
        writeKeyframePose(out, keyframe);
    }
}

GhostAnimationRecorder::~GhostAnimationRecorder() {}

GhostAnimationRecorder::GhostAnimationRecorder() {}
