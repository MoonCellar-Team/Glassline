#include "attribs.h"

#include <stddef.h>

#include <map>

#include "gl_proc.h"

#if defined(_WIN32)
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif

typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef char GLchar;

namespace {

template <class F>
F proc(const char* name) {
    return reinterpret_cast<F>(port::gfx::glProc(name));
}

unsigned s_wanted;
unsigned s_on;
GLuint s_program;
std::map<GLuint, unsigned> s_inputs;

void setArray(unsigned index, bool on) {
    static auto enable = proc<void (GL_APIENTRY*)(GLuint)>("glEnableVertexAttribArray");
    static auto disable = proc<void (GL_APIENTRY*)(GLuint)>("glDisableVertexAttribArray");
    if (on)
        enable(index);
    else
        disable(index);
}

unsigned inputsOf(GLuint program) {
    if (!program) return ~0u;
    auto it = s_inputs.find(program);
    if (it != s_inputs.end()) return it->second;
    static auto getProgram = proc<void (GL_APIENTRY*)(GLuint, GLenum, GLint*)>("glGetProgramiv");
    static auto active = proc<void (GL_APIENTRY*)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*)>("glGetActiveAttrib");
    static auto location = proc<GLint (GL_APIENTRY*)(GLuint, const GLchar*)>("glGetAttribLocation");
    GLint linked = 0, count = 0;
    getProgram(program, 0x8B82, &linked);
    if (!linked) return ~0u;
    getProgram(program, 0x8B89, &count);
    unsigned mask = 0;
    for (GLint i = 0; i < count; i++) {
        GLchar name[128];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        active(program, GLuint(i), GLsizei(sizeof name), &length, &size, &type, name);
        GLint loc = location(program, name);
        if (loc < 0) continue;
        int columns = type == 0x8B5A ? 2 : type == 0x8B5B ? 3 : type == 0x8B5C ? 4 : 1;
        for (int k = 0; k < columns * (size > 0 ? size : 1); k++)
            if (loc + k < 32)
                mask |= 1u << (loc + k);
            else
                mask = ~0u;
    }
    s_inputs[program] = mask;
    return mask;
}

void beforeDraw() {
    unsigned want = s_wanted & inputsOf(s_program);
    for (unsigned diff = want ^ s_on; diff; diff &= diff - 1) {
        unsigned index = unsigned(__builtin_ctz(diff));
        setArray(index, (want >> index) & 1);
    }
    s_on = want;
}

}

namespace port {
namespace gfx {

void releaseGameArrays() {
    for (unsigned on = s_on; on; on &= on - 1) setArray(unsigned(__builtin_ctz(on)), false);
    s_on = 0;
}

}
}

extern "C" {

void glEnableVertexAttribArray(GLuint index) {
    if (index >= 32) return setArray(index, true);
    s_wanted |= 1u << index;
}

void glDisableVertexAttribArray(GLuint index) {
    if (index >= 32) return setArray(index, false);
    s_wanted &= ~(1u << index);
    if ((s_on >> index) & 1) {
        setArray(index, false);
        s_on &= ~(1u << index);
    }
}

void glUseProgram(GLuint program) {
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glUseProgram");
    s_program = program;
    real(program);
}

void glLinkProgram(GLuint program) {
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glLinkProgram");
    s_inputs.erase(program);
    real(program);
}

void glDeleteProgram(GLuint program) {
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glDeleteProgram");
    s_inputs.erase(program);
    real(program);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLint, GLsizei)>("glDrawArrays");
    beforeDraw();
    real(mode, first, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLsizei, GLenum, const void*)>("glDrawElements");
    beforeDraw();
    real(mode, count, type, indices);
}

}
