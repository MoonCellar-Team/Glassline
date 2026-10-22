#include "m3g/LoaderHelpers.h"
#include "m3g/ShaderBinding.h"
#include <stddef.h>
void* operator new(size_t);
#include "m3g/Placement.h"
#include "runtime/midp/GLES11.h"
#include "runtime/midp/GLES20.h"
#include "runtime/midp/Graphics.h"
namespace m3g {
bool g_usingGLES20;

using LoaderObjects = util::VectorSE<midp::Object>;

void Loader_Mesh_preparePositionsForSkinning(Mesh* mesh) {
    mesh->preparePositionsForSkinning();
}
static midp::Object* getReference(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    midp::Object* object = objects->elementAt(readIntLE(in));
    if (roots && roots->contains(object)) roots->removeElement(object);
    return object;
}
static void loadObject3D(Object3D* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    object->setUserID(readIntLE(in));
    int count = readIntLE(in);
    for (int i = 0; i < count; ++i)
        object->addAnimationTrack(static_cast<AnimationTrack*>(getReference(in, objects, roots)));
    count = readIntLE(in);
    for (int i = 0; i < count; ++i) {
        readIntLE(in);
        int length = readIntLE(in);
        midp::array<signed char> bytes(length);
        in.readFully(bytes);
    }
}
static AnimationController* loadAnimationController(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    AnimationController* object = new AnimationController;
    loadObject3D(object, in, objects, roots);
    float speed = readFloatLE(in), weight = readFloatLE(in);
    int start = readIntLE(in), end = readIntLE(in);
    float position = readFloatLE(in);
    int time = readIntLE(in);
    object->setActiveInterval(start, end);
    object->setPosition(position, time);
    object->setSpeed(speed, time);
    object->setWeight(weight);
    return object;
}
static Appearance* loadAppearance(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    Appearance* object = new Appearance;
    loadObject3D(object, in, objects, roots);
    object->setLayer(static_cast<signed char>(in.readUnsignedByte()));
    object->setCompositingMode(static_cast<CompositingMode*>(getReference(in, objects, roots)));
    object->setFog(reinterpret_cast<Fog*>(getReference(in, objects, roots)));
    object->setPolygonMode(static_cast<PolygonMode*>(getReference(in, objects, roots)));
    object->setMaterial(reinterpret_cast<Material*>(getReference(in, objects, roots)));
    int count = readIntLE(in);
    for (int i = 0; i < count; ++i)
        object->setTexture(i, static_cast<Texture2D*>(getReference(in, objects, roots)));
    return object;
}
static void loadTransformable(Transformable* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    loadObject3D(object, in, objects, roots);
    if (in.readBoolean()) {
        float x = readFloatLE(in), y = readFloatLE(in), z = readFloatLE(in);
        object->setTranslation(x, y, z);
        x = readFloatLE(in); y = readFloatLE(in); z = readFloatLE(in);
        object->setScale(x, y, z);
        float angle = readFloatLE(in);
        x = readFloatLE(in); y = readFloatLE(in); z = readFloatLE(in);
        object->setOrientation(angle, x, y, z);
    }
    if (in.readBoolean()) {
        float matrix[16];
        for (int i = 0; i < 16; ++i) matrix[i] = readFloatLE(in);
        Transform* transform = new Transform;
        midp::INCREF(transform);
        transform->set(matrix);
        object->setTransform(transform);
        midp::DECREF(transform);
    }
}
static void loadNode(Node* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    loadTransformable(object, in, objects, roots);
    bool render = in.readBoolean(), pick = in.readBoolean();
    int alpha = in.readUnsignedByte();
    int scope = in.readInt();
    if (in.readBoolean()) {
        in.readUnsignedByte(); in.readUnsignedByte();
        getReference(in, objects, nullptr); getReference(in, objects, nullptr);
    }
    object->setRenderingEnable(render);
    object->setPickingEnable(pick);
    object->setAlphaFactor(static_cast<float>(alpha) * 0.003921568859368563f);
    object->setScope(scope);
}
static Camera* loadCamera(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    Camera* object = new Camera;
    loadNode(object, in, objects, roots);
    int projection = in.readUnsignedByte();
    if (projection == 0x30) {
        static_assert(sizeof(void*) != 4 || sizeof(Transform) == 0x50, "Transform");
        alignas(Transform) unsigned char storage[sizeof(Transform)];
        Transform* transform = new (storage) Transform;
        float matrix[16];
        for (int i = 0; i < 16; ++i) matrix[i] = readFloatLE(in);
        transform->set(matrix); object->setGeneric(transform);
        transform->~Transform();
    } else {
        float a = readFloatLE(in), b = readFloatLE(in), c = readFloatLE(in), d = readFloatLE(in);
        if (projection == 0x31) object->setParallel(a, b, c, d);
        else object->setPerspective(a, b, c, d);
    }
    return object;
}
static CompositingMode* loadCompositingMode(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    CompositingMode* object = new CompositingMode;
    loadObject3D(object, in, objects, roots);
    bool test = in.readBoolean(), depth = in.readBoolean(), color = in.readBoolean(), alpha = in.readBoolean();
    int blend = in.readUnsignedByte();
    float threshold = static_cast<float>(in.readUnsignedByte()) * 0.003921568859368563f;
    float factor = readFloatLE(in), units = readFloatLE(in);
    object->setDepthTestEnable(test); object->setDepthWriteEnable(depth);
    object->setColorWriteEnable(color); object->setAlphaWriteEnable(alpha);
    object->setBlending(blend); object->setAlphaThreshold(threshold); object->setDepthOffset(factor, units);
    return object;
}
static PolygonMode* loadPolygonMode(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    PolygonMode* object = new PolygonMode;
    loadObject3D(object, in, objects, roots);
    int cull = in.readUnsignedByte(), shade = in.readUnsignedByte(), winding = in.readUnsignedByte();
    bool two = in.readBoolean(), local = in.readBoolean(), perspective = in.readBoolean();
    object->setCulling(cull); object->setShading(shade); object->setWinding(winding);
    object->setTwoSidedLightingEnable(two); object->setLocalCameraLightingEnable(local);
    object->setPerspectiveCorrectionEnable(perspective);
    return object;
}
static void loadGroup(Group* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    loadNode(object, in, objects, roots);
    int count = readIntLE(in);
    for (int i = 0; i < count; ++i) object->addChild(static_cast<Node*>(getReference(in, objects, roots)));
}
static VertexBuffer* loadVertexBuffer(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    VertexBuffer* object = new VertexBuffer;
    loadObject3D(object, in, objects, roots);
    unsigned r = in.readUnsignedByte(), g = in.readUnsignedByte(), b = in.readUnsignedByte(), a = in.readUnsignedByte();
    object->setDefaultColor(static_cast<int>((a << 24) | (r << 16) | (g << 8) | b));
    VertexArray* positions = static_cast<VertexArray*>(getReference(in, objects, roots));
    midp::array<float> bias(3);
    for (int i = 0; i < 3; ++i) bias[i] = readFloatLE(in);
    float scale = readFloatLE(in);
    object->setPositions(positions, scale, bias);
    object->setNormals(static_cast<VertexArray*>(getReference(in, objects, roots)));
    object->setColors(static_cast<VertexArray*>(getReference(in, objects, roots)));
    int count = readIntLE(in);
    for (int unit = 0; unit < count; ++unit) {
        VertexArray* coords = static_cast<VertexArray*>(getReference(in, objects, roots));
        midp::array<float> texBias(3);
        for (int i = 0; i < 3; ++i) texBias[i] = readFloatLE(in);
        float texScale = readFloatLE(in);
        object->setTexCoords(unit, coords, texScale, texBias);
    }
    return object;
}
static World* loadWorld(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    World* object = new World;
    loadGroup(object, in, objects, roots);
    Camera* camera = static_cast<Camera*>(getReference(in, objects, roots));
    Background* background = reinterpret_cast<Background*>(getReference(in, objects, roots));
    object->setActiveCamera(camera); object->setBackground(background);
    return object;
}
void Loader_VertexArray_setFormat(VertexArray* object, int count, int components, int size) {
    object->setFormat(count, components, size);
}
VertexArray* Loader_createVertexArray() { return new VertexArray; }
static Object3D* loadVertexArray(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    VertexArray* object = Loader_createVertexArray();
    loadObject3D(object, in, objects, roots);
    int size = in.readUnsignedByte(), components = in.readUnsignedByte(), encoding = in.readUnsignedByte();
    int count = readShortLE(in);
    Loader_VertexArray_setFormat(object, count, components, size);
    if (encoding != 0) return object;
    int length = count * components;
    switch (size) {
    case 1: { midp::array<signed char> values(length);
        for (int i = 0; i < length; ++i) values[i] = in.readByte();
        object->set(0, count, values); break; }
    case 2: case 5: { midp::array<short> values(length);
        for (int i = 0; i < length; ++i) values[i] = readShortLE(in);
        object->set(0, count, values); break; }
    case 3: { midp::array<int> values(length);
        for (int i = 0; i < length; ++i) values[i] = readIntLE(in);
        object->set(0, count, values); break; }
    case 4: { midp::array<float> values(length);
        for (int i = 0; i < length; ++i) values[i] = readFloatLE(in);
        object->set(0, count, values); break; }
    }
    return object;
}
void Loader_TriangleStripArray_setStripLengths(TriangleStripArray* object, const midp::array<int>& lengths) { object->setStripLengths(lengths); }
void Loader_IndexBuffer_setFirstIndex(IndexBuffer* object, int index) { object->setFirstIndex(index); }
void Loader_IndexBuffer_setIndices(IndexBuffer* object, midp::array<int> indices) { object->setIndices(indices); }
static void loadIndexBuffer(IndexBuffer* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    loadObject3D(object, in, objects, roots);
    int encoding = in.readUnsignedByte();
    if (encoding < 128) {
        int first;
        if (encoding == 0) first = readIntLE(in);
        else if (encoding == 1) first = in.readUnsignedByte();
        else if (encoding == 2) first = readShortLE(in);
        else return;
        Loader_IndexBuffer_setFirstIndex(object, first);
    } else if (encoding >= 128 && encoding <= 130) {
        int count = readIntLE(in);
        midp::array<int> indices(count);
        for (int i = 0; i < count; ++i) {
            if (encoding == 128) indices[i] = readIntLE(in);
            else if (encoding == 129) indices[i] = in.readUnsignedByte();
            else indices[i] = readShortLE(in);
        }
        Loader_IndexBuffer_setIndices(object, indices);
    }
}
TriangleStripArray* Loader_createTriangleStripArray() { return new TriangleStripArray; }
static IndexBuffer* loadTriangleStripArray(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    TriangleStripArray* object = Loader_createTriangleStripArray();
    loadIndexBuffer(object, in, objects, roots);
    int count = readIntLE(in);
    midp::array<int> lengths(count);
    for (int i = 0; i < count; ++i) lengths[i] = readIntLE(in);
    Loader_TriangleStripArray_setStripLengths(object, lengths);
    return object;
}
Texture2D* Loader_createTexture2D() { return new Texture2D; }
static Transformable* loadTexture2D(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    Texture2D* object = Loader_createTexture2D();
    loadTransformable(object, in, objects, roots);
    object->setImage(static_cast<Image2D*>(getReference(in, objects, roots)));
    unsigned r = in.readUnsignedByte(), g = in.readUnsignedByte(), b = in.readUnsignedByte();
    int blend = in.readUnsignedByte(), wrapS = in.readUnsignedByte(), wrapT = in.readUnsignedByte();
    int level = in.readUnsignedByte(), image = in.readUnsignedByte();
    object->setBlendColor(static_cast<int>((r << 16) | (g << 8) | b));
    object->setBlending(blend); object->setWrapping(wrapS, wrapT); object->setFiltering(level, image);
    return object;
}
void Loader_SkinnedMesh_setLegacy(SkinnedMesh* object, bool legacy) { object->setLegacy(legacy); }
void Loader_SkinnedMesh_warmCache(SkinnedMesh* object) { object->getBoneTransforms(); }
void Loader_SkinnedMesh_setSkeleton(SkinnedMesh* object, Group* skeleton) { object->setSkeleton(skeleton); }
SkinnedMesh* Loader_createSkinnedMesh() { return new SkinnedMesh; }
void Loader_Mesh_setVertexBuffer(Mesh* object, VertexBuffer* vertices) { object->setVertexBuffer(vertices); }
void Loader_Mesh_setSubmeshCount(Mesh* object, int count) { object->setSubmeshCount(count); }
static void loadMesh(Mesh* object, midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    loadNode(object, in, objects, roots);
    Loader_Mesh_setVertexBuffer(object, static_cast<VertexBuffer*>(getReference(in, objects, roots)));
    int count = readIntLE(in);
    Loader_Mesh_setSubmeshCount(object, count);
    for (int i = 0; i < count; ++i) {
        object->setIndexBuffer(i, static_cast<IndexBuffer*>(getReference(in, objects, roots)));
        object->setAppearance(i, static_cast<Appearance*>(getReference(in, objects, roots)));
    }
}
static Mesh* loadSkinnedMesh(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    SkinnedMesh* object = Loader_createSkinnedMesh();
    loadMesh(object, in, objects, roots);
    Loader_Mesh_preparePositionsForSkinning(object);
    Loader_SkinnedMesh_setSkeleton(object, static_cast<Group*>(getReference(in, objects, roots)));
    Loader_SkinnedMesh_setLegacy(object, true);
    int count = readIntLE(in);
    for (int i = 0; i < count; ++i) {
        Node* bone = static_cast<Node*>(getReference(in, objects, roots));
        int first = readIntLE(in), vertices = readIntLE(in), weight = readIntLE(in);
        object->addTransform(bone, weight, first, vertices);
    }
    Loader_SkinnedMesh_warmCache(object);
    return object;
}
Mesh* Loader_createMesh() { return new Mesh; }
void Loader_KeyframeSequence_setKeyframeSize(KeyframeSequence* object, int count, int components) { object->setKeyframeSize(count, components); }
void Loader_KeyframeSequence_setInterpolation(KeyframeSequence* object, int interpolation) { object->setInterpolation(interpolation); }
KeyframeSequence* Loader_createKeyframeSequence() { return new KeyframeSequence; }
static Object3D* loadKeyframeSequence(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    KeyframeSequence* object = Loader_createKeyframeSequence();
    loadObject3D(object, in, objects, roots);
    int interpolation = in.readUnsignedByte(), repeat = in.readUnsignedByte(), encoding = in.readUnsignedByte();
    int duration = readIntLE(in), start = readIntLE(in), end = readIntLE(in);
    int components = readIntLE(in), count = readIntLE(in);
    Loader_KeyframeSequence_setInterpolation(object, interpolation);
    object->setRepeatMode(repeat); object->setDuration(duration);
    Loader_KeyframeSequence_setKeyframeSize(object, count, components);
    object->setValidRange(start, end);
    if (encoding == 0) {
        midp::array<float> values(components);
        for (int i = 0; i < count; ++i) {
            int time = readIntLE(in);
            for (int j = 0; j < components; ++j) values[j] = readFloatLE(in);
            object->setKeyframe(i, time, values);
        }
    } else if (encoding == 2) {
        float bias[4], scale[4];
        for (int j = 0; j < components; ++j) bias[j] = readFloatLE(in);
        for (int j = 0; j < components; ++j) scale[j] = readFloatLE(in);
        midp::array<float> values(components);
        for (int i = 0; i < count; ++i) {
            int time = readIntLE(in);
            for (int j = 0; j < components; ++j) {
                float value = static_cast<float>(readUShortLE(in));
                values[j] = scale[j] * (value * 0.000015259021893143654f) + bias[j];
            }
            object->setKeyframe(i, time, values);
        }
    }
    return object;
}
void Loader_Image2D_commit(Image2D* object, const signed char* data) { object->commit(reinterpret_cast<const unsigned char*>(data)); }
void Loader_Image2D_commit(Image2D* object, const midp::array<signed char>& palette, const signed char* data) { object->commit(palette, data); }
void Loader_Image2D_set(Image2D* object, int format, int width, int height) { object->set(format, width, height); }
Image2D* Loader_createImage2D() { return new Image2D; }
static Object3D* loadImage2D(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    Image2D* object = Loader_createImage2D();
    loadObject3D(object, in, objects, roots);
    int format = in.readUnsignedByte(); bool mutableImage = in.readBoolean();
    int width = readIntLE(in), height = readIntLE(in);
    Loader_Image2D_set(object, format, width, height);
    if (!mutableImage) {
        midp::array<signed char> palette;
        int paletteSize = readIntLE(in);
        if (paletteSize > 0) { palette = midp::array<signed char>(paletteSize); in.readFully(palette); }
        int size = readIntLE(in);
        signed char* data = new signed char[static_cast<unsigned>(size)];
        in.readFully(data, size);
        if (palette != nullptr) Loader_Image2D_commit(object, palette, data);
        else Loader_Image2D_commit(object, data);
        delete[] data;
    }
    return object;
}
void Loader_AnimationTrack_setProperty(AnimationTrack* object, int property) { object->setProperty(property); }
void Loader_AnimationTrack_setKeyframeSequence(AnimationTrack* object, KeyframeSequence* sequence) { object->setKeyframeSequence(sequence); }
AnimationTrack* Loader_createAnimationTrack() { return new AnimationTrack; }
static Object3D* loadAnimationTrack(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    AnimationTrack* object = Loader_createAnimationTrack();
    loadObject3D(object, in, objects, roots);
    KeyframeSequence* sequence = static_cast<KeyframeSequence*>(getReference(in, objects, roots));
    AnimationController* controller = static_cast<AnimationController*>(getReference(in, objects, roots));
    int property = readIntLE(in);
    Loader_AnimationTrack_setKeyframeSequence(object, sequence);
    Loader_AnimationTrack_setProperty(object, property); object->setController(controller);
    return object;
}
static bool loadObject(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    int type = in.read();
    if (type == -1) return false;
    int size = readIntLE(in);
    midp::DataInputStream data(new ChunkInputStream(reinterpret_cast<midp::InputStream*>(&in), size));
    Object3D* object = nullptr;
    switch (type) {
    case 0: {
        data.readUnsignedByte(); data.readUnsignedByte(); data.readBoolean();
        readIntLE(data); readIntLE(data);
        unsigned length = static_cast<unsigned>(size) - 11;
        signed char* bytes = new signed char[length];
        data.readFully(bytes, static_cast<int>(length)); delete[] bytes;
        break;
    }
    case 1: object = loadAnimationController(data, objects, roots); break;
    case 2: object = loadAnimationTrack(data, objects, roots); break;
    case 3: object = loadAppearance(data, objects, roots); break;
    case 5: object = loadCamera(data, objects, roots); break;
    case 6: object = loadCompositingMode(data, objects, roots); break;
    case 8: object = loadPolygonMode(data, objects, roots); break;
    case 9: { Group* group = new Group; loadGroup(group, data, objects, roots); object = group; break; }
    case 10: object = loadImage2D(data, objects, roots); break;
    case 11: object = loadTriangleStripArray(data, objects, roots); break;
    case 14: { Mesh* mesh = Loader_createMesh(); loadMesh(mesh, data, objects, roots); object = mesh; break; }
    case 16: object = loadSkinnedMesh(data, objects, roots); break;
    case 17: object = loadTexture2D(data, objects, roots); break;
    case 19: object = loadKeyframeSequence(data, objects, roots); break;
    case 20: object = loadVertexArray(data, objects, roots); break;
    case 21: object = loadVertexBuffer(data, objects, roots); break;
    case 22: object = loadWorld(data, objects, roots); break;
    }
    objects->addElement(object);
    if (object) roots->addElement(object);
    return true;
}
bool loadSection(midp::DataInputStream& in, LoaderObjects* objects, LoaderObjects* roots) {
    if (in.read() == -1) return false;
    int length = readIntLE(in);
    readIntLE(in);
    midp::DataInputStream data(new ChunkInputStream(reinterpret_cast<midp::InputStream*>(&in), length - 13));
    while (loadObject(data, objects, roots)) {}
    in.skip(4);
    return true;
}
int reverseBytes(int value) { return static_cast<int>(__builtin_bswap32(static_cast<unsigned>(value))); }
short reverseBytes(short value) { return static_cast<short>(__builtin_bswap16(static_cast<unsigned short>(value))); }
int readIntLE(midp::DataInputStream& in) { return reverseBytes(in.readInt()); }
float readFloatLE(midp::DataInputStream& in) {
    int bits = readIntLE(in);
    float value;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
}
short readShortLE(midp::DataInputStream& in) { return reverseBytes(in.readShort()); }
unsigned short readUShortLE(midp::DataInputStream& in) { return static_cast<unsigned short>(readShortLE(in)); }
}

#include "m3g/VertexHelpers.h"

namespace m3g {
void VFPMatrixMultiply(const float* destination, const float* left, float* right)
{
    float a[16], b[16];
    for (int i = 0; i < 16; ++i) { a[i] = left[i]; b[i] = right[i]; }
    float* out = const_cast<float*>(destination);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = a[row] * b[column * 4];
            value += a[row + 4] * b[column * 4 + 1];
            value += a[row + 8] * b[column * 4 + 2];
            value += a[row + 12] * b[column * 4 + 3];
            out[column * 4 + row] = value;
        }
    }
}

template<> const float fdot3<4>(const float* a, const float* b)
{
    return (a[0] * b[0] + a[1] * b[4]) + a[2] * b[8];
}

int componentSize(int type)
{
    switch (type) {
        case 1: return 1;
        case 2: case 5: return 2;
        case 3: case 4: return 4;
        default: return 0;
    }
}

template<> void __getVerts<float, float>(float* out, const float* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<float, signed char>(float* out, const signed char* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<float, short>(float* out, const short* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<signed char, signed char>(signed char* out, const signed char* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<int, int>(int* out, const int* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<short, short>(short* out, const short* source,
    int first, int count, int components, int stride)
{
    unsigned negative = 0u - static_cast<unsigned>(count);
    if (static_cast<int>(negative) < 0) negative = ~0u;
    unsigned remaining = static_cast<unsigned>(count) + negative + 1u;
    source += first * stride;
    do {
        out[0] = source[0];
        out[1] = source[1];
        if (components >= 3) {
            out[2] = source[2];
            if (components >= 4) out[3] = source[3];
        }
        out += components < 3 ? 2 : (components < 4 ? 3 : 4);
        source += stride;
    } while (--remaining != 0);
}

template<> void __getVerts<signed char, signed char>(signed char* out, int, const signed char* source,
    int first, int count, int components, int stride)
{
    __getVerts<signed char, signed char>(out, source, first, count, components, stride);
}

template<> void __getVerts<int, int>(int* out, int, const int* source,
    int first, int count, int components, int stride)
{
    __getVerts<int, int>(out, source, first, count, components, stride);
}

template<> void __getVerts<short, short>(short* out, int, const short* source,
    int first, int count, int components, int stride)
{
    __getVerts<short, short>(out, source, first, count, components, stride);
}

template<> void __setVerts<signed char, signed char>(signed char* destination, int, const signed char* source,
    int first, int count, int components, int stride)
{
    if (count <= 0) return;
    destination += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            destination[component] = *source++;
        destination += stride;
    }
}

template<> void __setVerts<float, float>(float* destination, int, const float* source,
    int first, int count, int components, int stride)
{
    if (count <= 0) return;
    destination += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            destination[component] = *source++;
        destination += stride;
    }
}

template<> void __setVerts<int, int>(int* destination, int, const int* source,
    int first, int count, int components, int stride)
{
    if (count <= 0) return;
    destination += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            destination[component] = *source++;
        destination += stride;
    }
}

template<> void __setVerts<short, short>(short* destination, int, const short* source,
    int first, int count, int components, int stride)
{
    if (count <= 0) return;
    destination += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            destination[component] = *source++;
        destination += stride;
    }
}

template<> void __getVertsTransform<float, const int, fixedToFloat>(float* out, int,
    const int* source, int first, int count, int components, int stride, fixedToFloat convert)
{
    if (count <= 0) return;
    source += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            *out++ = convert(source[component]);
        source += stride;
    }
}

template<> void __getVertsTransform<float, const short, halfToFloat>(float* out, int,
    const short* source, int first, int count, int components, int stride, halfToFloat convert)
{
    if (count <= 0) return;
    source += first * stride;
    for (int vertex = 0; vertex < count; ++vertex) {
        for (int component = 0; component < components; ++component)
            *out++ = convert(source[component]);
        source += stride;
    }
}

bool verifyTextureUnit(int unit)
{
    return unit >= 0 && unit < 2;
}

void ensureNoGLError()
{
}

template<> void scaleAndBiasVertex<3, short>(const float* scaleBias, float* out, const short* source)
{
    float scale = scaleBias[0];
    short x = source[0], y = source[1], z = source[2];
    out[0] = static_cast<float>(x) * scale + scaleBias[1];
    out[1] = static_cast<float>(y) * scale + scaleBias[2];
    out[2] = static_cast<float>(z) * scale + scaleBias[3];
}

template<> void scaleAndBiasVertex<3, float>(const float* scaleBias, float* out, const float* source)
{
    float scale = scaleBias[0];
    out[0] = source[0] * scale + scaleBias[1];
    out[1] = source[1] * scale + scaleBias[2];
    out[2] = source[2] * scale + scaleBias[3];
}

void transpose(float* matrix)
{
    float value = matrix[1]; matrix[1] = matrix[4]; matrix[4] = value;
    value = matrix[2]; matrix[2] = matrix[8]; matrix[8] = value;
    value = matrix[3]; matrix[3] = matrix[12]; matrix[12] = value;
    value = matrix[6]; matrix[6] = matrix[9]; matrix[9] = value;
    value = matrix[7]; matrix[7] = matrix[13]; matrix[13] = value;
    value = matrix[11]; matrix[11] = matrix[14]; matrix[14] = value;
}

void vfpTransformBeta(const float* source, float* out, float weight, const float* matrix)
{
    float result[4];
    const float x = source[0], y = source[1], z = source[2];
    for (int row = 0; row < 4; ++row) {
        float value = matrix[row] * x;
        value += matrix[row + 4] * y;
        value += matrix[row + 8] * z;
        result[row] = (matrix[row + 12] + value) * weight;
    }
    for (int row = 0; row < 4; ++row) out[row] = result[row];
}

void vfpTransformBetaAccumulate(const float* source, float* out, float weight, const float* matrix)
{
    float result[4];
    const float x = source[0], y = source[1], z = source[2];
    for (int row = 0; row < 4; ++row) {
        float value = matrix[row] * x;
        value += matrix[row + 4] * y;
        value += matrix[row + 8] * z;
        result[row] = out[row] + (matrix[row + 12] + value) * weight;
    }
    for (int row = 0; row < 4; ++row) out[row] = result[row];
}

bool isZeroWeight(int weight)
{
    return (static_cast<unsigned>(weight) & 0x70000000u) == 0;
}
}

#include "m3g/GLHelpers.h"
#include "m3g/VertexHelpers.h"

extern "C" {
void glTexParameterx(unsigned, unsigned, int);
void glTexParameteri(unsigned, unsigned, int);
void glVertexPointer(int, unsigned, int, const void*);
void glTexCoordPointer(int, unsigned, int, const void*);
void glDrawArrays(unsigned, int, int);
void glVertexAttribPointer(unsigned, int, unsigned, unsigned char, int, const void*);
void glGetFloatv(unsigned, float*);
void glCurrentPaletteMatrixOES(unsigned);
void glLoadMatrixf(const float*);
void glMultMatrixf(const float*);
void glTranslatef(float, float, float);
void glScalef(float, float, float);
}

namespace m3g {
namespace {
int objectType(const Object3D* object) {
    return object->getM3GUniqueClassID();
}
}
template<> AnimationController* const m3g_cast<AnimationController>(Object3D* object) {
    return objectType(object) == 1 ? reinterpret_cast<AnimationController*>(object) : 0;
}
template<> const Group* const m3g_cast<Group>(const Object3D* object) {
    int type = objectType(object);
    return type == 9 || type == 22 ? reinterpret_cast<const Group*>(object) : 0;
}
template<> Group* const m3g_cast<Group>(Object3D* object) {
    int type = objectType(object);
    return type == 9 || type == 22 ? reinterpret_cast<Group*>(object) : 0;
}
template<> Image2D* const m3g_cast<Image2D>(Object3D* object) {
    return objectType(object) == 10 ? reinterpret_cast<Image2D*>(object) : 0;
}
void* implementation_Graphics3D_getRendererForTarget(midp::Object* target) {
    return target ? static_cast<midp::GraphicsIPhoneGLES*>(target)->createRenderer() : 0;
}
Graphics3DIPhone* implementation_Graphics3D_getInstance() {
    static Graphics3DIPhone* g3d;
    if (!g3d) {
        g3d = new Graphics3DIPhone;
        midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(g3d));
    }
    return g3d;
}
int alignOffset(int offset) {
    unsigned remainder = static_cast<unsigned>(offset) & 3;
    if (remainder) offset = static_cast<unsigned>(offset) + 4 - remainder;
    return offset;
}
namespace {
template<class T> void copySharedVertices(const VertexArray* source, VertexArray* target, int count, int components) {
    midp::array<T> values(count * components);
    source->get(0, count, values);
    target->set(0, count, values);
}
}
VertexArray* createSharedVertexArray(const VertexArray* source, void* storage, int offset, int stride) {
    int count = source->getVertexCount();
    int components = source->getComponentCount();
    int type = source->getComponentType();
    VertexArray* target = new VertexArray(count, components, type, storage, offset, stride);
    switch (type) {
    case 1: copySharedVertices<signed char>(source, target, count, components); break;
    case 2: case 5: copySharedVertices<short>(source, target, count, components); break;
    case 3: copySharedVertices<int>(source, target, count, components); break;
    case 4: copySharedVertices<float>(source, target, count, components); break;
    }
    return target;
}
template<> const Mesh* const m3g_cast<Mesh>(const Object3D* object) {
    int type = objectType(object);
    return type == 14 || type == 16 ? reinterpret_cast<const Mesh*>(object) : 0;
}
template<> Mesh* const m3g_cast<Mesh>(Object3D* object) {
    int type = objectType(object);
    return type == 14 || type == 16 ? reinterpret_cast<Mesh*>(object) : 0;
}
template<> Node* const m3g_cast<Node>(Object3D* object) {
    unsigned type = objectType(object);
    return type <= 22 && ((1u << type) & 0x455220u) ? reinterpret_cast<Node*>(object) : 0;
}
bool getPathToParent(const Node* node, const Node* parent, util::VectorSE<Node>* path) {
    const Node* previous;
    do {
        previous = node;
        if (node == parent) break;
        path->addElement(const_cast<Node*>(node));
        node = node->getParent();
    } while (node);
    return previous == parent;
}
template<> const SkinnedMesh* const m3g_cast<SkinnedMesh>(const Object3D* object) {
    return objectType(object) == 16 ? reinterpret_cast<const SkinnedMesh*>(object) : 0;
}
template<> SkinnedMesh* const m3g_cast<SkinnedMesh>(Object3D* object) {
    return objectType(object) == 16 ? reinterpret_cast<SkinnedMesh*>(object) : 0;
}
namespace {
#ifdef MEDECOMP_PORT
extern "C" int medecomp_anisotropy(void);
extern "C" int medecomp_texture_replace(const void* data, int size, int width, int height, int format, unsigned target);
extern "C" int medecomp_enlarge_sign(const void* data, int size, int width, int height, int format, unsigned target,
                                    int minFilter, int wrapS, int wrapT);
void setAnisotropy(void (*parameter)(unsigned, unsigned, int), int min) {
    int level = medecomp_anisotropy();
    if (level == 0 || parameter != glTexParameteri) return;
    parameter(0xde1, 0x84fe, min >= 0x2700 && min <= 0x2703 ? level : 1);
}
#endif
template<class GL>
void bindImageTexture(GL* gl, Image2D* image, int target, int mag, int min, int wrapS, int wrapT,
                      void (*parameter)(unsigned, unsigned, int), void (*destroy)(Image2DPlatformData*)) {
    Image2DPlatformData* platform = image->m_platform;
    int texture = platform->texture;
    if (!texture || platform->dirty) {
        bool update = platform->dirty && texture;
        platform->dirty = false;
        unsigned format = image->getFormat();
        if (update) {
            gl->glBindTexture(target, texture);
        } else {
            gl->glGenTextures(1, reinterpret_cast<unsigned int*>(&texture));
            gl->glBindTexture(target, texture);
            parameter(target, 0x2800, mag);
            parameter(target, 0x2801, min);
#ifdef MEDECOMP_PORT
            setAnisotropy(parameter, min);
#endif
            parameter(0xde1, 0x2802, wrapS);
            parameter(0xde1, 0x2803, wrapT);
        }
        int internal = 0x1907;
        int pixelFormat = 0x1907;
        int type = 0x1401;
        bool uncompressed = true;
        bool packedMipmaps = false;
        switch (format & 255) {
        case 99: gl->glPixelStorei(0xcf5, 1); break;
        case 100:
            gl->glPixelStorei(0xcf5, 4);
            internal = pixelFormat = 0x1908;
            break;
        case 101:
            type = 0x8363;
            gl->glPixelStorei(0xcf5, 2);
            break;
        case 102: case 103:
            type = (format & 255) == 102 ? 0x8034 : 0x8033;
            gl->glPixelStorei(0xcf5, 2);
            internal = pixelFormat = 0x1908;
            break;
        case 120: case 121: case 122: case 123: case 124: case 125: {
            gl->glPixelStorei(0xcf5, 1);
            static const int formats[] = {0x8c92, 0x8c93, 0x8c01, 0x8c03, 0x8c00, 0x8c02};
            internal = formats[(format & 255) - 120];
            packedMipmaps = (format & 0x8000) != 0;
            uncompressed = false;
            break;
        }
        default: break;
        }
#ifdef MEDECOMP_PORT
        if (!update && !image->isMutable() &&
            medecomp_texture_replace(image->getMipMapData(0), image->getMipMapDataSize(0), image->getMipMapWidth(0),
                                     image->getMipMapHeight(0), image->getFormat(), unsigned(target))) {
        } else if (!update && !image->isMutable() &&
                   medecomp_enlarge_sign(image->getMipMapData(0), image->getMipMapDataSize(0), image->getMipMapWidth(0),
                                         image->getMipMapHeight(0), image->getFormat(), unsigned(target), min, wrapS,
                                         wrapT)) {
        } else
#endif
        if (packedMipmaps) {
            int width = image->getMipMapWidth(0);
            int height = image->getMipMapHeight(0);
            const void* data = image->getMipMapData(0);
            int size = image->getMipMapDataSize(0);
            for (int level = 0;; ++level) {
                if (update) gl->glCompressedTexSubImage2D(target, level, 0, 0, width, height, 0x1907, size, data);
                else gl->glCompressedTexImage2D(target, level, internal, width, height, 0, size, data);
                if (width == 1) break;
                int nextSize = size >> 2;
                size = nextSize >= 32 ? nextSize : 32;
                width >>= 1;
                height >>= 1;
            }
        } else {
            int uploadLevel = 0;
            for (int level = 0; level < image->getNumMipMapLevels(); ++level) {
                int width = image->getMipMapWidth(level);
                int height = image->getMipMapHeight(level);
#ifdef MEDECOMP_PORT
                if (width > 4096 || height > 4096) continue;
#else
                if (width > 1024 || height > 1024) continue;
#endif
                const void* data = image->getMipMapData(level);
                int size = image->getMipMapDataSize(level);
                if (uncompressed) {
                    if (update) gl->glTexSubImage2D(target, uploadLevel, 0, 0, width, height, pixelFormat, type, data);
                    else gl->glTexImage2D(target, uploadLevel, internal, width, height, 0, pixelFormat, type, data);
                } else {
                    if (update) gl->glCompressedTexSubImage2D(target, uploadLevel, 0, 0, width, height, pixelFormat, size, data);
                    else gl->glCompressedTexImage2D(target, uploadLevel, internal, width, height, 0, size, data);
                }
                ++uploadLevel;
            }
        }
        if (!image->isMutable()) image->discard();
        platform->destructor = destroy;
        platform->texture = texture;
        platform->magFilter = mag;
        platform->minFilter = min;
        platform->wrapS = wrapS;
        platform->wrapT = wrapT;
    }
    gl->glBindTexture(target, texture);
    if (platform->magFilter != mag) { parameter(0xde1, 0x2800, mag); platform->magFilter = mag; }
    if (platform->minFilter != min) {
        parameter(0xde1, 0x2801, min);
        platform->minFilter = min;
#ifdef MEDECOMP_PORT
        setAnisotropy(parameter, min);
#endif
    }
    if (platform->wrapS != wrapS) { parameter(0xde1, 0x2802, wrapS); platform->wrapS = wrapS; }
    if (platform->wrapT != wrapT) { parameter(0xde1, 0x2803, wrapT); platform->wrapT = wrapT; }
}
}
void Image2DBindGLES(midp::GLES11* gl, Image2D* image, int target, int mag, int min, int s, int t) {
    bindImageTexture(gl, image, target, mag, min, s, t, glTexParameterx, glesTextureDestructor);
}
void glesTextureDestructor(Image2DPlatformData* data) {
    int texture = data->texture;
    if (!texture) return;
    data->texture = 0;
    glesRemoveTexture(texture);
}
void Image2DFlushGLES(midp::GLES11* gl, Image2D* image) {
    gl->glActiveTexture(0x84c0);
    gl->glClientActiveTexture(0x84c0);
    Image2DBindGLES(gl, image, 0xde1, 0x2601, 0x2601, 0x2901, 0x2901);
    short vertices[4] = {};
    static const float tc[8] = {0, 0, 1, 0, 1, 1, 0, 0};
    gl->glDisableClientState(0x8076);
    gl->glDisableClientState(0x8075);
    gl->glEnableClientState(0x8074);
    glVertexPointer(2, 0x1402, 0, vertices);
    glTexCoordPointer(2, 0x1406, 0, tc);
    gl->glEnableClientState(0x8078);
    glDrawArrays(5, 0, 4);
    gl->glDisableClientState(0x8074);
    gl->glDisableClientState(0x8078);
}
void glesRemoveTexture(int texture) {
    if (texture) midp::OpenGLESManager::removeTexture(texture);
}
template<int Components> void applySkinWeights(int, float*, float*, const float*, const signed char*, const float*, const Transform* const*);
template<> void applySkinWeights<3>(int count, float* output, float* scratch, const float* vertex,
                                  const signed char* bones, const float* weights, const Transform* const* transforms) {
    output[0] = output[1] = output[2] = 0.0f;
    for (int i = 0; i < count; ++i) {
        float weight = weights[i];
        if (weight == 0.0f) break;
        scratch[0] = vertex[0]; scratch[1] = vertex[1]; scratch[2] = vertex[2]; scratch[3] = 1.0f;
        transforms[static_cast<unsigned char>(bones[i])]->transform(scratch, 4);
        output[0] += scratch[0] * weight;
        output[1] += scratch[1] * weight;
        output[2] += scratch[2] * weight;
    }
}
void uploadTransformPalette(const midp::array<Transform*>* palette, const float* scaleBias, int renderer, midp::GLES11*) {
    midp::GLES11* gl = reinterpret_cast<midp::GLES11*>(renderer);
    int count = palette->m_length;
    float scale = scaleBias[0], x = scaleBias[1], y = scaleBias[2], z = scaleBias[3];
    gl->glMatrixMode(0x1700);
    float modelview[16];
    glGetFloatv(0xba6, modelview);
    gl->glEnable(0x8840);
    gl->glMatrixMode(0x8840);
    for (int i = 0; i < count; ++i) {
        glCurrentPaletteMatrixOES(i);
        glLoadMatrixf(modelview);
        float matrix[16];
        (*palette)[i]->get(matrix);
        transpose(matrix);
        glMultMatrixf(matrix);
        glTranslatef(x, y, z);
        glScalef(scale, scale, scale);
    }
}
void Image2DBindGLES20(midp::GLES20* gl, Image2D* image, int target, int mag, int min, int s, int t) {
    bindImageTexture(gl, image, target, mag, min, s, t, glTexParameteri, gles20TextureDestructor);
}
void gles20TextureDestructor(Image2DPlatformData* data) {
    int texture = data->texture;
    if (!texture) return;
    data->texture = 0;
    gles20RemoveTexture(texture);
}
void Image2DFlushGLES20(midp::GLES20* gl, Image2D* image) {
    gl->glActiveTexture(0x84c0);
    Image2DBindGLES20(gl, image, 0xde1, 0x2601, 0x2601, 0x2901, 0x2901);
    ShaderBinding* shader = static_cast<ShaderBinding*>(
        OpenGLES20ShaderSupport::getInstance()->getStockShader(static_cast<StockES2Shaders::StockES2Shaders>(5)));
    int program = shader->m_program;
    int sampler = *reinterpret_cast<int*>(shader->m_textureBindings);
    gl->glUseProgram(program);
    gl->glUniform1i(sampler, 0);
    gl->glDisableVertexAttribArray(6);
    gl->glDisableVertexAttribArray(2);
    gl->glDisableVertexAttribArray(1);
    gl->glEnableVertexAttribArray(0);
    short vertices[9] = {};
    static const float tc[6] = {0, 0, 1, 0, 1, 1};
    glVertexAttribPointer(0, 3, 0x1402, 0, 0, vertices);
    glVertexAttribPointer(5, 2, 0x1406, 0, 0, tc);
    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(5);
    glDrawArrays(4, 0, 1);
    gl->glDisableVertexAttribArray(0);
    gl->glDisableVertexAttribArray(5);
}
void gles20RemoveTexture(int texture) {
    if (texture) midp::OpenGLESManager::removeTexture(texture);
}
}
