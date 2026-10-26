#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5_webgl.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "gl_proc.h"

namespace port {
namespace gfx {

#ifdef __EMSCRIPTEN__
thread_local bool t_glCurrent;
#endif

void* glProc(const char* name) {
#ifdef __EMSCRIPTEN__
    void* f = emscripten_webgl_get_proc_address(name);
#else
    void* f = SDL_GL_GetProcAddress(name);
#endif
    if (!f) {
        fprintf(stderr, "medecomp: в драйвере GL нет %s\n", name);
        abort();
    }
    return f;
}

}
}
