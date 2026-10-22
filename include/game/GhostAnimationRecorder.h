#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/util/VectorSE.h"

class GhostKeyframe;
namespace midp {
class String;
}

class GhostAnimationRecorder : public midp::ReferenceCounted {
public:
    GhostAnimationRecorder();
    virtual ~GhostAnimationRecorder();

    void reset();
    void addKeyframe(GhostKeyframe* keyframe);
    void write(midp::ReferenceCountedPointer<midp::String> filename);

    util::VectorSE<GhostKeyframe> m_keyframes;
};
