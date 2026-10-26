#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace port::platform {
inline bool defaultTouchInput() {
#ifdef __ANDROID__
    return true;
#elif defined(__EMSCRIPTEN__)
    static const bool touch = MAIN_THREAD_EM_ASM_INT({
        return navigator.maxTouchPoints > 0 && window.matchMedia("(pointer: coarse)").matches;
    }) != 0;
    return touch;
#else
    return false;
#endif
}
}
