#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class AnimationTrackClass : public midp::Class {
public:
    AnimationTrackClass();
    virtual ~AnimationTrackClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
