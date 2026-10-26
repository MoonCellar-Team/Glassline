#pragma once

namespace port {
namespace gfx {

void* glProc(const char* name);

#ifdef __EMSCRIPTEN__
extern thread_local bool t_glCurrent;
#define MEDECOMP_GL_GUARD(...) \
    if (!port::gfx::t_glCurrent) return __VA_ARGS__
#else
#define MEDECOMP_GL_GUARD(...)
#endif

}
}
