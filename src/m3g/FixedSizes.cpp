#include <stddef.h>

#include "game/AnimPlayer3D.h"
#include "game/GameCamera.h"
#include "game/IFFReader.h"
#include "game/IFFWriter.h"
#include "game/MEdgeMap.h"
#include "game/SeperatedAxesList.h"
#include "m3g/GZipStreamBasic.h"
#include "m3g/Mesh.h"
#include "m3g/ShaderBinding.h"
#include "m3g/Transform.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/FontIPhone.h"
#include "runtime/midp/Runtime.h"
#include "runtime/util/VectorSE.h"

static_assert(sizeof(void*) != 4 || sizeof(util::VectorSE<midp::Object>) == 0x18, "VectorSE");
static_assert(sizeof(void*) != 4 || sizeof(midp::DataInputStream) == 0x10, "DataInputStream");
static_assert(sizeof(void*) != 4 || sizeof(midp::DataOutputStream) == 0xc, "DataOutputStream");
static_assert(sizeof(void*) != 4 || sizeof(m3g::Transform) == 0x50, "Transform");
static_assert(sizeof(void*) != 4 || sizeof(AnimPlayer3D) == 0x2c, "AnimPlayer3D");
static_assert(sizeof(void*) != 4 || sizeof(MEdgeMap) == 0x1b4, "MEdgeMap");
static_assert(sizeof(void*) != 4 || offsetof(MEdgeMap, m_mapGroup) == 0x134, "MEdgeMap vectors");
static_assert(sizeof(void*) != 4 || sizeof(SeperatedAxesList) == 0x24, "SeperatedAxesList");
static_assert(sizeof(void*) != 4 || offsetof(SeperatedAxesList, m_count) == 0x20, "SeperatedAxesList::m_axes");
static_assert(sizeof(void*) != 4 || sizeof(midp::Runtime) == 0x78, "Runtime");
static_assert(sizeof(void*) != 4 || offsetof(midp::Runtime, m_pointers) == 0x20, "Runtime::m_timers");
static_assert(sizeof(void*) != 4 || offsetof(midp::Runtime, m_midlets) == 0x60, "Runtime::m_midlets");
static_assert(sizeof(void*) != 4 || sizeof(IFFReader) == 0x1c, "IFFReader");
static_assert(sizeof(void*) != 4 || offsetof(IFFReader, m_chunkSize) == 0x18, "IFFReader::m_in");
static_assert(sizeof(void*) != 4 || sizeof(IFFWriter) == 0x1c, "IFFWriter");
static_assert(sizeof(void*) != 4 || sizeof(GameCamera) == 0x7a0, "GameCamera");
static_assert(sizeof(void*) != 4 || offsetof(GameCamera, m_aspectRatio) == 0x720, "GameCamera::m_transform");
static_assert(sizeof(void*) != 4 || offsetof(GameCamera, m_lookFromAnim) == 0x75c, "GameCamera::m_lookAtAnim");
static_assert(sizeof(void*) != 4 || sizeof(m3g::ShaderBinding) == 0x28, "ShaderBinding");
static_assert(sizeof(void*) != 4 || sizeof(midp::FontIPhoneCGTTF::GlyphMap) == 0x18, "GlyphMap");
static_assert(sizeof(void*) != 4 || sizeof(m3g::Mesh) == 0x130, "Mesh");
static_assert(sizeof(void*) != 4 || offsetof(m3g::Mesh, m_mutable) == 0x128, "Mesh::m_mutable");
static_assert(sizeof(m3g::MeshIndexBuffers) >= sizeof(util::VectorSE<midp::Object>), "Mesh indices");
static_assert(sizeof(m3g::MeshAppearances) >= sizeof(util::VectorSE<midp::Object>), "Mesh appearances");
static_assert(offsetof(m3g::GZipStreamBasic, m_blockSize) - offsetof(m3g::GZipStreamBasic, m_zState) == 32, "m_zState");
