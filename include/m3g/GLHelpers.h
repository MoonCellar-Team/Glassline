#pragma once
#include "m3g/Image2D.h"
#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/Graphics3DIPhone.h"
#include "m3g/VertexArray.h"
#include "m3g/Node.h"
#include "m3g/Image2DPlatformData.h"
#include "m3g/Mesh.h"
#include "m3g/Transform.h"
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"

namespace midp {
class GLES11;
class GLES20;
class OpenGLESManager { public: static void removeTexture(int); static void flushDeletedTextures(); static void removeBuffer(int); static GLES11* getGLES(); };
}
namespace m3g {
class AnimationController;
class SkinnedMesh;
template<class T> T* const m3g_cast(Object3D*);
template<class T> const T* const m3g_cast(const Object3D*);
void glesRemoveTexture(int);
void gles20RemoveTexture(int);
void glesTextureDestructor(Image2DPlatformData*);
void gles20TextureDestructor(Image2DPlatformData*);
void Image2DBindGLES(midp::GLES11*, Image2D*, int, int, int, int, int);
void Image2DBindGLES20(midp::GLES20*, Image2D*, int, int, int, int, int);
}
