#include "game/SceneClass.h"

#include <stddef.h>

#include "runtime/midp/String.h"

midp::Object* SceneClass::newInstance() const {
    return NULL;
}

SceneClass::SceneClass() {}

SceneClass::~SceneClass() {}

midp::String* SceneClass::getName() const {
    return new midp::String("game.Scene");
}
