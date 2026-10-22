#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "gl_proc.h"

namespace port {
namespace gfx {

void* glProc(const char* name) {
    void* f = SDL_GL_GetProcAddress(name);
    if (!f) {
        fprintf(stderr, "medecomp: в драйвере GL нет %s\n", name);
        abort();
    }
    return f;
}

}
}
