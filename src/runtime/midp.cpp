#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/Runtime.h"
#include "runtime/midp/Thread.h"
#include "runtime/midp/array.h"

#include "sdk/Foundation.h"
#include "sdk/OpenAL.h"
#include "sdk/vector.h"

#include <math.h>
#include <stddef.h>

namespace midp {

void INCREF(ReferenceCounted* obj) {
    if (obj != NULL) {
        obj->_incRef();
    }
}

bool DECREF(ReferenceCounted* obj) {
    if (obj != NULL && obj->_decRef()) {
        delete obj;
        return true;
    }
    return false;
}

template <class T>
static ReferenceCounted* refCounted(T* p) {
    return p;
}

template <>
array<array<int>> array2D<int>(int rows, int cols) {
    array<array<int>> result(rows);
    if (rows > 0) {
        for (int i = 0; i < rows; i++) {
            array<int> row(cols);
            result[i] = row;
        }
    }
    return result;
}

void* Thread_WebImageRun(void* arg) {
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    static_cast<WebImage*>(arg)->run();
    pool->release();
    return NULL;
}

GraphicsIPhoneCG* implementation_Image_getGraphics(Image* image) {
    ImageIPhone* iphone = static_cast<ImageIPhone*>(image);
    iphone->invalidate();
    return new GraphicsIPhoneCG(iphone->m_context);
}

ImageIPhone* implementation_Image_createImage(int width, int height) {
    return new ImageIPhone(width, height);
}

ImageIPhone* implementation_Image_createImage(InputStream* stream) {
    return new ImageIPhone(stream);
}

}

static CGRect CGRectMake(float x, float y, float, float) {
    CGRect rect;
    rect.origin.x = x;
    rect.origin.y = y;
    rect.size.width = 768.0f;
    rect.size.height = 1024.0f;
    return rect;
}

void propListener(void*, unsigned long, unsigned long, const void*) {}

namespace midp {

void* Thread_posixRun(void* arg) {
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    static_cast<Thread*>(arg)->run();
    static_cast<Thread*>(arg)->setDead();
    DECREF(refCounted(static_cast<Thread*>(arg)));
    pool->release();
    return NULL;
}

MIDlet* Graphics3D_getMIDlet() {
    return Runtime::getRuntime()->getCurrentMIDlet();
}

void DECREF_NO_DELETE(ReferenceCounted* obj) {
    if (obj != NULL) {
        obj->_decRef();
    }
}

unsigned int CalculatePacketForTime(AudioStreamBasicDescription& format, double seconds) {
    const double packets =
        (format.mSampleRate / static_cast<double>(format.mFramesPerPacket)) * seconds;
    if (!(packets > 0.0))
        return 0;
    if (packets >= 4294967296.0)
        return 0xffffffffu;
    return static_cast<unsigned int>(packets);
}

void CalculateBytesForTime(AudioStreamBasicDescription& format, unsigned long maxPacketSize,
                           double seconds, unsigned long* outBufferSize,
                           unsigned long* outNumPackets) {
    unsigned long buffer;
    if (format.mFramesPerPacket != 0)
        buffer = CalculatePacketForTime(format, seconds) * maxPacketSize;
    else if (maxPacketSize < 0x20000ul)
        buffer = 0x20000ul;
    else
        buffer = maxPacketSize;
    *outBufferSize = buffer;
    if (*outBufferSize > 0x20000ul) {
        if (*outBufferSize > maxPacketSize)
            *outBufferSize = 0x20000ul;
    } else if (*outBufferSize <= 15ul) {
        *outBufferSize = 16ul;
    }
    *outNumPackets = *outBufferSize / maxPacketSize;
}

void AudioQueue_property(void* user, OpaqueAudioQueue* queue, unsigned long property) {
    INCREF(refCounted(static_cast<PlayerIPhoneCoreAudio*>(user)));
    static_cast<PlayerIPhoneCoreAudio*>(user)->propertyCB(queue, property);
    DECREF(refCounted(static_cast<PlayerIPhoneCoreAudio*>(user)));
}

void AudioQueue_playback(void* user, OpaqueAudioQueue* queue, AudioQueueBuffer* buffer) {
    INCREF(refCounted(static_cast<PlayerIPhoneCoreAudio*>(user)));
    if (static_cast<Player*>(user)->getState() == 400)
        static_cast<PlayerIPhoneCoreAudio*>(user)->playbackCB(queue, buffer, true);
    DECREF(refCounted(static_cast<PlayerIPhoneCoreAudio*>(user)));
}

struct SourcePool {
    bool ready;
    std::vector<unsigned int> freeIds;
};
static SourcePool g_sourcePool;
static_assert(offsetof(SourcePool, freeIds) == alignof(void*), "vector сразу после флага");

void sourcePoolInit() {
    if (!g_sourcePool.ready)
        g_sourcePool.ready = true;
}

unsigned int sourcePoolGet() {
    if (g_sourcePool.freeIds.empty()) {
        unsigned int id = 0;
        alGenSources(1, &id);
        if (id == 0)
            return 0;
        g_sourcePool.freeIds.push_back(id);
    }
    const unsigned int id = g_sourcePool.freeIds.back();
    g_sourcePool.freeIds.pop_back();
    return id;
}

void sourcePoolPut(unsigned int id) {
    g_sourcePool.freeIds.push_back(id);
}

void makeArcPath(CGContext* ctx, int x, int y, int width, int height, int startAngle, int arcAngle) {
    const unsigned int degBits = 0x3c8efa33u;
    float degreesToRadians;
    __builtin_memcpy(&degreesToRadians, &degBits, sizeof degreesToRadians);

    const float rx = static_cast<float>(width) * 0.5f;
    const float ry = static_cast<float>(height) * 0.5f;
    const float cx = static_cast<float>(x) + rx;
    const float cy = static_cast<float>(y) + ry;
    const float startRad = static_cast<float>(startAngle) * degreesToRadians;
    const float endRad = static_cast<float>(startAngle + arcAngle) * degreesToRadians;

    CGContextBeginPath(ctx);
    const double startSin = sin(static_cast<double>(startRad));
    const double startCos = cos(static_cast<double>(startRad));
    float prevX = static_cast<float>(static_cast<double>(cx) + startCos * static_cast<double>(rx));
    float prevY = static_cast<float>(static_cast<double>(cy) + startSin * static_cast<double>(ry));
    CGContextMoveToPoint(ctx, prevX, prevY);

    const int segments = arcAngle / 90 + 1;
    if (segments < 1)
        return;

    const float rxk = rx * 0.25f;
    const float ryk = ry * 0.25f;
    float prevCos = cosf(startRad);
    float prevSinNeg = -sinf(startRad);
    const float segmentCount = static_cast<float>(segments);
    for (int i = 1; i <= segments; i++) {
        const float t = static_cast<float>(i) / segmentCount;
        const float angle = (1.0f - t) * startRad + t * endRad;
        const double angleSin = sin(static_cast<double>(angle));
        const float angleCosF = cosf(angle);
        const double angleCos = cos(static_cast<double>(angle));
        const float angleSinF = sinf(angle);
        const float nextX =
            static_cast<float>(static_cast<double>(cx) + angleCos * static_cast<double>(rx));
        const float nextY =
            static_cast<float>(static_cast<double>(cy) + angleSin * static_cast<double>(ry));
        const float cp1x = prevX + prevSinNeg * rxk;
        const float cp1y = prevY + prevCos * ryk;
        const float cp2x = nextX + angleSinF * rxk;
        const float cp2y = nextY - angleCosF * ryk;
        CGContextAddCurveToPoint(ctx, cp1x, cp1y, cp2x, cp2y, nextX, nextY);
        prevX = nextX;
        prevY = nextY;
        prevCos = angleCosF;
        prevSinNeg = -angleSinF;
    }
}

bool g_invalidProgramState = true;

void invalidateGraphicsIPhoneShaderState() {
    g_invalidProgramState = true;
}

void transpose(float* m) {
    float t = m[1];
    m[1] = m[4];
    m[4] = t;
    t = m[2];
    m[2] = m[8];
    m[8] = t;
    t = m[3];
    m[3] = m[12];
    m[12] = t;
    t = m[6];
    m[6] = m[9];
    m[9] = t;
    t = m[7];
    m[7] = m[13];
    m[13] = t;
    t = m[11];
    m[11] = m[14];
    m[14] = t;
}

unsigned long InputStreamGetBytes(void* info, void* buffer, unsigned long count) {
    const int n = static_cast<InputStream*>(info)->read(static_cast<signed char*>(buffer),
                                                        static_cast<int>(count));
    if (n == -1)
        return 0;
    return static_cast<unsigned long>(n);
}

long long InputStreamSkipBytes(void* info, long long count) {
    return static_cast<InputStream*>(info)->skip(static_cast<long>(count));
}

void InputStreamRewind(void* info) {
    static_cast<InputStream*>(info)->reset();
}

void InputStreamRelease(void* info) {
    DECREF(static_cast<ReferenceCounted*>(info));
}

void* CGDataProviderCreateWithInputStream(InputStream* stream) {
    INCREF(stream);
    CGDataProviderSequentialCallbacks callbacks;
    callbacks.version = 0;
    callbacks.getBytes = InputStreamGetBytes;
    callbacks.skipForward = InputStreamSkipBytes;
    callbacks.rewind = InputStreamRewind;
    callbacks.releaseInfo = InputStreamRelease;
    return CGDataProviderCreateSequential(stream, &callbacks);
}

}
