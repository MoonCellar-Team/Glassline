#include "m3g/AnimationTrackClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

AnimationTrackClass::AnimationTrackClass() {}

AnimationTrackClass::~AnimationTrackClass() {}

midp::String* AnimationTrackClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.AnimationTrack");
}

midp::Object* AnimationTrackClass::newInstance() const { return reinterpret_cast<midp::Object*>(new AnimationTrack); }

}
