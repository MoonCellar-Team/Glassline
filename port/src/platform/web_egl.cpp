#ifdef __EMSCRIPTEN__

#include <EGL/egl.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <stdint.h>

#include <initializer_list>

#include "../gfx/gl_proc.h"

namespace {

EGLint s_error = EGL_SUCCESS;
EGLDisplay const kDisplay = reinterpret_cast<EGLDisplay>(1);
EGLConfig const kConfig = reinterpret_cast<EGLConfig>(1);
EGLSurface const kSurface = reinterpret_cast<EGLSurface>(1);
const EGLint kContextMajor = 0x3098;

EGLBoolean fail(EGLint error) {
    s_error = error;
    return EGL_FALSE;
}

}

extern "C" {

EGLDisplay eglGetDisplay(EGLNativeDisplayType) { return kDisplay; }

EGLBoolean eglInitialize(EGLDisplay, EGLint* major, EGLint* minor) {
    if (major) *major = 1;
    if (minor) *minor = 4;
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay) { return EGL_TRUE; }

EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig* configs, EGLint size, EGLint* count) {
    if (configs && size > 0) configs[0] = kConfig;
    if (count) *count = 1;
    return EGL_TRUE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay, EGLConfig, EGLint attribute, EGLint* value) {
    if (!value) return fail(EGL_BAD_PARAMETER);
    switch (attribute) {
    case EGL_RED_SIZE:
    case EGL_GREEN_SIZE:
    case EGL_BLUE_SIZE:
    case EGL_ALPHA_SIZE: *value = 8; break;
    case EGL_BUFFER_SIZE: *value = 32; break;
    case EGL_DEPTH_SIZE: *value = 24; break;
    case EGL_STENCIL_SIZE: *value = 8; break;
    case EGL_RENDERABLE_TYPE:
    case EGL_CONFORMANT: *value = EGL_OPENGL_ES2_BIT | 0x40; break;
    case EGL_SURFACE_TYPE: *value = EGL_WINDOW_BIT; break;
    case EGL_CONFIG_ID: *value = 1; break;
    case EGL_CONFIG_CAVEAT: *value = EGL_NONE; break;
    default: *value = 0; break;
    }
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*) { return kSurface; }

EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface) { return EGL_TRUE; }

EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface, EGLint attribute, EGLint* value) {
    int w = 0, h = 0;
    emscripten_get_canvas_element_size("#canvas", &w, &h);
    if (attribute == EGL_WIDTH) *value = w;
    else if (attribute == EGL_HEIGHT) *value = h;
    else return fail(EGL_BAD_ATTRIBUTE);
    return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint* attribs) {
    int major = 2;
    for (int i = 0; attribs && attribs[i] != EGL_NONE; i += 2)
        if (attribs[i] == kContextMajor) major = attribs[i + 1];
    EmscriptenWebGLContextAttributes a;
    emscripten_webgl_init_context_attributes(&a);
    a.alpha = false;
    a.antialias = false;
    a.premultipliedAlpha = false;
    a.enableExtensionsByDefault = true;
    a.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    for (int webgl : {2, 1}) {
        if (webgl == 1 && major >= 3) break;
        a.majorVersion = webgl;
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE h = emscripten_webgl_create_context("#canvas", &a);
        if (h > 0) return reinterpret_cast<EGLContext>(intptr_t(h));
    }
    s_error = EGL_BAD_MATCH;
    return EGL_NO_CONTEXT;
}

EGLBoolean eglDestroyContext(EGLDisplay, EGLContext context) {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE h = EMSCRIPTEN_WEBGL_CONTEXT_HANDLE(intptr_t(context));
    if (emscripten_webgl_get_current_context() == h) {
        emscripten_webgl_make_context_current(0);
        port::gfx::t_glCurrent = false;
    }
    emscripten_webgl_destroy_context(h);
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface, EGLSurface, EGLContext context) {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE h = EMSCRIPTEN_WEBGL_CONTEXT_HANDLE(intptr_t(context));
    if (emscripten_webgl_make_context_current(h) != EMSCRIPTEN_RESULT_SUCCESS) return fail(EGL_BAD_CONTEXT);
    port::gfx::t_glCurrent = h != 0;
    return EGL_TRUE;
}

EGLContext eglGetCurrentContext(void) {
    return reinterpret_cast<EGLContext>(intptr_t(emscripten_webgl_get_current_context()));
}

EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface) { return EGL_TRUE; }
EGLBoolean eglSwapInterval(EGLDisplay, EGLint) { return EGL_TRUE; }
EGLBoolean eglWaitNative(EGLint) { return EGL_TRUE; }
EGLBoolean eglWaitGL(void) { return EGL_TRUE; }
EGLBoolean eglBindAPI(EGLenum api) { return api == EGL_OPENGL_ES_API ? EGL_TRUE : fail(EGL_BAD_PARAMETER); }

const char* eglQueryString(EGLDisplay, EGLint name) {
    switch (name) {
    case EGL_VENDOR: return "WebGL";
    case EGL_VERSION: return "1.4";
    case EGL_CLIENT_APIS: return "OpenGL_ES";
    default: return "";
    }
}

EGLint eglGetError(void) {
    EGLint e = s_error;
    s_error = EGL_SUCCESS;
    return e;
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* name) {
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(emscripten_webgl_get_proc_address(name));
}

}

#endif
