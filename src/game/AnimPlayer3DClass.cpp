#include "game/AnimPlayer3DClass.h"

#include <stddef.h>

#include "runtime/midp/String.h"

midp::Object* AnimPlayer3DClass::newInstance() const {
    return NULL;
}

AnimPlayer3DClass::AnimPlayer3DClass() {}

AnimPlayer3DClass::~AnimPlayer3DClass() {}

midp::String* AnimPlayer3DClass::getName() const {
    return new midp::String("game.AnimPlayer3D");
}
