#pragma once
#include "m3g/SkinnedMesh.h"
#include "m3g/VertexBuffer.h"
#include "m3g/TriangleStripArray.h"
#include "m3g/Appearance.h"
#include "m3g/Texture2D.h"
#include "m3g/ChunkInputStream.h"
#include "m3g/GLHelpers.h"
#include "m3g/PolygonMode.h"
#include "m3g/CompositingMode.h"
#include "m3g/AnimationTrack.h"
#include "m3g/AnimationController.h"
#include "m3g/Node.h"
#include "m3g/Mesh.h"
#include "m3g/Transform.h"
#include "m3g/Camera.h"
#include "m3g/World.h"
#include "m3g/KeyframeSequence.h"
#include "runtime/midp/array.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/InputStream.h"
#include "runtime/util/VectorSE.h"
namespace m3g {
class Fog; class Material; class Background;
int reverseBytes(int); short reverseBytes(short);
int readIntLE(midp::DataInputStream&); float readFloatLE(midp::DataInputStream&);
short readShortLE(midp::DataInputStream&); unsigned short readUShortLE(midp::DataInputStream&);
}

namespace m3g {
class SkinnedMesh;
void Loader_SkinnedMesh_warmCache(SkinnedMesh* object);
}
