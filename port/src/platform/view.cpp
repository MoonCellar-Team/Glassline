#include "port/platform.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/GLES.h"

namespace {
const int GL_FRAMEBUFFER = 0x8D40;
}

namespace port {
namespace platform {

View& view() {
    static View v;
    return v;
}

void beginFrame();

void bind() {
    acquireContext();
    beginFrame();
    view().gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer());
}

void flush() {
    present();
    yieldContext();
}

void draw() {
    bind();
    view().display->refresh();
    flush();
}

}
}

extern "C" {

int HostView_getGLESMajorVersion(void* self) MEDECOMP_SYMBOL("_ZN12_GLOBAL__N_18HostView19getGLESMajorVersionEv");
int HostView_getGLESMajorVersion(void* self) { return static_cast<port::platform::View*>(self)->glesMajor; }

int HostView_getGLESMinorVersion(void* self) MEDECOMP_SYMBOL("_ZN12_GLOBAL__N_18HostView19getGLESMinorVersionEv");
int HostView_getGLESMinorVersion(void* self) { return static_cast<port::platform::View*>(self)->glesMinor; }

void* HostView_getGL(void* self) MEDECOMP_SYMBOL("_ZN12_GLOBAL__N_18HostView5getGLEv");
void* HostView_getGL(void* self) { return static_cast<port::platform::View*>(self)->gl; }

void HostView_draw(void* self) MEDECOMP_SYMBOL("_ZN12_GLOBAL__N_18HostView4drawEv");
void HostView_draw(void*) { port::platform::draw(); }

}
