#pragma once

#define GL_GLES_PROTOTYPES 0
#include <GLES2/gl2.h>

namespace port {
namespace platform {

#define GL_FUNCS(X)                                                                                         \
    X(ACTIVETEXTURE, ActiveTexture) X(ATTACHSHADER, AttachShader) X(BINDATTRIBLOCATION, BindAttribLocation) \
    X(BINDBUFFER, BindBuffer) X(BINDFRAMEBUFFER, BindFramebuffer) X(BINDRENDERBUFFER, BindRenderbuffer)      \
    X(BINDTEXTURE, BindTexture) X(CHECKFRAMEBUFFERSTATUS, CheckFramebufferStatus) X(CLEAR, Clear)            \
    X(CLEARCOLOR, ClearColor) X(COLORMASK, ColorMask) X(COMPILESHADER, CompileShader)                        \
    X(CREATEPROGRAM, CreateProgram) X(CREATESHADER, CreateShader) X(DISABLE, Disable)                        \
    X(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray) X(DRAWARRAYS, DrawArrays) X(ENABLE, Enable)        \
    X(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray) X(FRAMEBUFFERRENDERBUFFER, FramebufferRenderbuffer)  \
    X(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D) X(GENFRAMEBUFFERS, GenFramebuffers)                        \
    X(GENRENDERBUFFERS, GenRenderbuffers) X(GENTEXTURES, GenTextures) X(GETBOOLEANV, GetBooleanv)            \
    X(GETFLOATV, GetFloatv) X(GETINTEGERV, GetIntegerv) X(GETPROGRAMIV, GetProgramiv)                        \
    X(GETSHADERINFOLOG, GetShaderInfoLog) X(GETSHADERIV, GetShaderiv) X(GETSTRING, GetString)                \
    X(GETUNIFORMLOCATION, GetUniformLocation) X(GETVERTEXATTRIBIV, GetVertexAttribiv) X(ISENABLED, IsEnabled) \
    X(LINKPROGRAM, LinkProgram) X(READPIXELS, ReadPixels) X(RENDERBUFFERSTORAGE, RenderbufferStorage)       \
    X(SHADERSOURCE, ShaderSource)                                                                           \
    X(TEXIMAGE2D, TexImage2D) X(TEXPARAMETERI, TexParameteri) X(UNIFORM1I, Uniform1i)                        \
    X(UNIFORM1F, Uniform1f) X(UNIFORM2F, Uniform2f) X(UNIFORM4F, Uniform4f) X(DELETETEXTURES, DeleteTextures) \
    X(USEPROGRAM, UseProgram) X(VERTEXATTRIBPOINTER, VertexAttribPointer) X(VIEWPORT, Viewport)            \
    X(DEPTHMASK, DepthMask) X(BLENDFUNCSEPARATE, BlendFuncSeparate) X(BLENDEQUATION, BlendEquation)        \
    X(SCISSOR, Scissor) X(DRAWELEMENTS, DrawElements) X(TEXSUBIMAGE2D, TexSubImage2D)                     \
    X(PIXELSTOREI, PixelStorei)

#define GL_DECLARE(U, N) PFNGL##U##PROC N;
struct GLFuncs {
    GL_FUNCS(GL_DECLARE)
};
#undef GL_DECLARE
extern GLFuncs gl;

typedef void(GL_APIENTRYP PfnRenderbufferStorageMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void(GL_APIENTRYP PfnBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield,
                                              GLenum);
extern PfnRenderbufferStorageMultisample glRenderbufferStorageMultisample_;
extern PfnBlitFramebuffer glBlitFramebuffer_;
const GLenum kReadFramebuffer = 0x8ca8, kDrawFramebuffer = 0x8ca9, kMaxSamples = 0x8d57, kRgba8 = 0x8058;
const GLenum kDepth24 = 0x81a6;
bool postNeedsDepth();

extern GLint s_attrPos, s_attrUV;
GLuint link(const char* vs, const char* fs);

}
}
