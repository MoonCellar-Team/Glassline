#include "game/BGMusicClass.h"

#include <stddef.h>

#include "runtime/midp/String.h"

midp::Object* BGMusicClass::newInstance() const {
    return NULL;
}

midp::String* BGMusicClass::getName() const {
    return new midp::String("game.BGMusic");
}

BGMusicClass::BGMusicClass() {}

BGMusicClass::~BGMusicClass() {}
