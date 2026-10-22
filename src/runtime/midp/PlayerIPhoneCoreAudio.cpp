#include "runtime/midp/Player.h"

#include "platform/AudioLock.h"
#include "runtime/midp/Long.h"
#include "runtime/midp/PlayerIPhoneCoreAudioClass.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"
#include "sdk/Foundation.h"

#ifdef MEDECOMP_PORT
#include <set>

namespace {
std::set<const void*> s_pausedQueues;
}
#endif

const int kPrefetched = 300;
const int kInvalidRunState = -0x10476;
const unsigned kDataFormat = 0x64666d74;
const unsigned kPacketCount = 0x70636e74;
const unsigned kPacketUpperBound = 0x706b7562;
const unsigned kMagicCookie = 0x6d676963;
const unsigned kChannelLayout = 0x636d6170;
const unsigned kQueueMagicCookie = 0x61716d63;
const unsigned kQueueChannelLayout = 0x6171636c;
const unsigned kQueueIsRunning = 0x6171726e;
const unsigned kQueueSampleRate = 0x61717372;
const unsigned kDefaultScale = 0x37be37c6;

struct AQBuffer {
    unsigned capacity;
    void* data;
    unsigned byteSize;
    void* user;
    unsigned packetCapacity;
    void* packets;
    unsigned packetCount;
};

struct QueueBits {
    OpaqueAudioQueue* queue;
    midp::OpaqueAudioQueueTimeline* timeline;
    int count;
    AudioQueueBuffer** buffers;
    midp::PlayerIPhoneCoreAudio* player;
};

class AudioQueueAction : public NSObject {
public:
    static AudioQueueAction* alloc();
    AudioQueueAction* init();
    void pauseQueue_(void* queue);
    void startQueue_(void* queue);
};

class QueueObject : public NSObject {
public:
    static QueueObject* alloc();
    QueueObject* initWithQueue_(OpaqueAudioQueue* queue);
};

class AudioQueueStarter : public NSObject {
public:
    static AudioQueueStarter* alloc();
    AudioQueueStarter* init();
    void startPlayer_(void* player);
};

class PlayerObject : public NSObject {
public:
    static PlayerObject* alloc();
    PlayerObject* initWithPlayer_(midp::PlayerIPhoneCoreAudio* player);
};

class AudioQueueDeallocator : public NSObject {
public:
    static AudioQueueDeallocator* alloc();
    AudioQueueDeallocator* init();
    void deallocateQueue_(void* info);
};

class AudioQueueCloser : public NSObject {
public:
    static AudioQueueCloser* alloc();
    AudioQueueCloser* init();
    void closeQueue_(void* info);
};

class AudioQueueStructObject : public NSObject {
public:
    static AudioQueueStructObject* alloc();
    AudioQueueStructObject* initWithAudioQueue_(QueueBits* info);
};

namespace {

extern "C" int AudioQueueGetParameter(OpaqueAudioQueue* queue, unsigned param, float* value);
extern "C" int AudioQueueSetParameter(OpaqueAudioQueue* queue, unsigned param, float value);
extern "C" int AudioQueueFlush(OpaqueAudioQueue* queue);
extern "C" int AudioQueueStop(OpaqueAudioQueue* queue, unsigned immediate);
extern "C" int AudioQueueStart(OpaqueAudioQueue* queue, const void* startTime);
extern "C" int AudioQueuePrime(OpaqueAudioQueue* queue, unsigned frames, unsigned* prepared);
extern "C" int AudioQueueNewOutput(const AudioStreamBasicDescription* format, void (*callback)(),
                                   void* user, const void* runLoop, const void* mode,
                                   unsigned flags, OpaqueAudioQueue** outQueue);
extern "C" int AudioQueueCreateTimeline(OpaqueAudioQueue* queue,
                                        midp::OpaqueAudioQueueTimeline** out);
extern "C" int AudioQueueGetProperty(OpaqueAudioQueue* queue, unsigned property, void* data,
                                     unsigned* size);
extern "C" int AudioQueueSetProperty(OpaqueAudioQueue* queue, unsigned property, const void* data,
                                     unsigned size);
extern "C" int AudioQueueAddPropertyListener(OpaqueAudioQueue* queue, unsigned property,
                                             void (*callback)(), void* user);
extern "C" int AudioQueueAllocateBufferWithPacketDescriptions(OpaqueAudioQueue* queue,
                                                              unsigned bytes, unsigned packets,
                                                              AudioQueueBuffer** out);
extern "C" int AudioQueueEnqueueBuffer(OpaqueAudioQueue* queue, AudioQueueBuffer* buffer,
                                       unsigned count, const void* descs);
extern "C" int AudioQueueGetCurrentTime(OpaqueAudioQueue* queue,
                                        midp::OpaqueAudioQueueTimeline* timeline,
                                        double* time, unsigned char* discontinuity);
extern "C" int AudioFileGetPropertyInfo(OpaqueAudioFileID* file, unsigned property,
                                        unsigned* size, unsigned* writable);
extern "C" int AudioFileReadPackets(OpaqueAudioFileID* file, int useCache, unsigned* outBytes,
                                    void* descs, long long startPacket, unsigned* ioPackets,
                                    void* outBuffer);
extern "C" int AudioFileClose(OpaqueAudioFileID* file);
extern "C" int AudioSessionSetActive(unsigned active);
extern "C" void* memset(void* dst, int value, size_t size);
extern "C" void* malloc(size_t size);
extern "C" void free(void* ptr);
extern "C" const void* kCFRunLoopCommonModes;

AQBuffer* asBuffer(AudioQueueBuffer* buffer) {
    return reinterpret_cast<AQBuffer*>(buffer);
}

}

namespace midp {

VolumeControl* PlayerIPhoneCoreAudio::getVolumeControl() const {
    return const_cast<VolumeControl*>(static_cast<const VolumeControl*>(this));
}

bool PlayerIPhoneCoreAudio::isExclusive() const {
    return false;
}

void PlayerIPhoneCoreAudio::update(bool exclusive) {
    if (getState() != STARTED)
        return;
    if (m_exclusiveHeld == exclusive)
        return;
    m_exclusiveHeld = true;
    AudioQueueAction* action = AudioQueueAction::alloc()->init();
    QueueObject* queue = QueueObject::alloc()->initWithQueue_(m_queue);
    if (exclusive)
        action->pauseQueue_(queue);
    else
        action->startQueue_(queue);
    action->release();
}

int PlayerIPhoneCoreAudio::getVolume() const {
    if (m_queue) {
        float level = 0;
        AudioQueueGetParameter(m_queue, 1, &level);
        m_volume = (int)(level * 100.0f);
    }
    return m_volume;
}

void PlayerIPhoneCoreAudio::setVolume(int volume) {
    m_volume = volume;
    if (m_queue)
        AudioQueueSetParameter(m_queue, 1, (float)m_volume * 0.01f);
    notifyListeners(PlayerListener::VOLUME_CHANGED, 0);
}

void PlayerIPhoneCoreAudio::start() {
    Player::start();
    if (getState() == STARTED)
        return;
    stateTransition(STARTED);
    if (!m_queue)
        return;
#ifdef MEDECOMP_PORT
    if (s_pausedQueues.erase(this)) {
        AudioLock lock;
        AudioQueueStart(m_queue, 0);
        AudioQueueSetParameter(m_queue, 1, (float)m_volume * 0.01f);
        return;
    }
#endif
    AudioQueueStarter* starter = AudioQueueStarter::alloc()->init();
    PlayerObject* player = PlayerObject::alloc()->initWithPlayer_(this);
    starter->startPlayer_(player);
    starter->release();
}

void PlayerIPhoneCoreAudio::stop() {
    if (getState() != STARTED)
        return;
    stateTransition(kPrefetched);
    if (m_queue) {
        AudioQueueAction* action = AudioQueueAction::alloc()->init();
        QueueObject* queue = QueueObject::alloc()->initWithQueue_(m_queue);
        action->pauseQueue_(queue);
        action->release();
#ifdef MEDECOMP_PORT
        s_pausedQueues.insert(this);
#endif
    }
    Long* time = new Long(getMediaTime());
    INCREF(time);
    notifyListeners(PlayerListener::STOPPED, time);
    DECREF(time);
}

long long PlayerIPhoneCoreAudio::setMediaTime(long long time) {
    if (getState() == STARTED)
        AudioQueueFlush(m_queue);
#ifdef MEDECOMP_PORT
    if (s_pausedQueues.erase(this) && m_queue) {
        AudioLock lock;
        AudioQueueFlush(m_queue);
        AudioQueueStop(m_queue, 1);
    }
#endif
    m_timeOffset = (int)time;
    m_packet = (int)CalculatePacketForTime(m_format, (double)time * 1e-6);
    return getMediaTime();
}

void PlayerIPhoneCoreAudio::realize() {
    if (getState() != UNREALIZED || m_queue)
        return;
    unsigned size = 0x28;
    AudioFileGetProperty(m_file, kDataFormat, &size, &m_format);
    m_scale = (float)(1.0 / m_format.mSampleRate);
    AudioQueueNewOutput(&m_format, reinterpret_cast<void (*)()>(AudioQueue_playback), this,
                        0, kCFRunLoopCommonModes, 0, &m_queue);
    AudioQueueCreateTimeline(m_queue, &m_timeline);
    setContentType(new String("audio/x-caf"));
    stateTransition(REALIZED);
}

long long PlayerIPhoneCoreAudio::getDuration() const {
    if (m_duration == -1) {
        unsigned size = 0;
        unsigned writable = 0;
        AudioFileGetPropertyInfo(m_file, kPacketCount, &size, &writable);
        unsigned long long packets = 0;
        AudioFileGetProperty(m_file, kPacketCount, &size, &packets);
        float frames = (float)(packets * m_format.mFramesPerPacket);
        m_duration = (long long)((double)(frames * m_scale) * 1000000.0);
    }
    return m_duration;
}

void PlayerIPhoneCoreAudio::deallocate() {
    int state = getState();
    if (state == CLOSED)
        return;
    if (state == STARTED)
        stop();
    if (getState() != kPrefetched)
        return;
    AudioLock lock;
    if (!m_queue) {
        stateTransition(REALIZED);
        return;
    }
    int count = m_bufferCount;
    AudioQueueBuffer** copy = new AudioQueueBuffer*[count];
    for (int i = 0; i < count; i++) {
        copy[i] = m_buffers[i];
        m_buffers[i] = 0;
    }
    stateTransition(REALIZED);
    QueueBits* info = new QueueBits;
    info->queue = m_queue;
    info->count = count;
    info->buffers = copy;
    info->player = this;
    AudioQueueDeallocator* deallocator = AudioQueueDeallocator::alloc()->init();
    AudioQueueStructObject* object = AudioQueueStructObject::alloc()->initWithAudioQueue_(info);
    deallocator->deallocateQueue_(object);
    deallocator->release();
}

void PlayerIPhoneCoreAudio::prefetch() {
    Player::prefetch();
    if (getState() != REALIZED || !m_buffers || m_buffers[0])
        return;
    unsigned size = 4;
    unsigned maxPacket = 0;
    AudioFileGetProperty(m_file, kPacketUpperBound, &size, &maxPacket);
    unsigned long bufferBytes = 0;
    unsigned long descs = 0;
    CalculateBytesForTime(m_format, maxPacket, 0.5, &bufferBytes, &descs);
    m_packetDescs = (unsigned)descs;
    size = 4;
    if (AudioFileGetPropertyInfo(m_file, kMagicCookie, &size, 0) == 0 && size != 0) {
        char* cookie = new char[size];
        AudioFileGetProperty(m_file, kMagicCookie, &size, cookie);
        AudioQueueSetProperty(m_queue, kQueueMagicCookie, cookie, size);
        delete[] cookie;
    }
    size = 4;
    if (AudioFileGetPropertyInfo(m_file, kChannelLayout, &size, 0) == 0 && size != 0) {
        void* layout = malloc(size);
        AudioFileGetProperty(m_file, kChannelLayout, &size, layout);
        AudioQueueSetProperty(m_queue, kQueueChannelLayout, layout, size);
        free(layout);
    }
    AudioQueueAddPropertyListener(m_queue, kQueueIsRunning,
                                  reinterpret_cast<void (*)()>(AudioQueue_property), this);
    INCREF(this);
    bool variable = m_format.mBytesPerPacket == 0 || m_format.mFramesPerPacket == 0;
    for (int i = 0; i < m_bufferCount; i++) {
        unsigned descs = variable ? m_packetDescs : 0;
        AudioQueueAllocateBufferWithPacketDescriptions(m_queue, bufferBytes, descs, &m_buffers[i]);
    }
    stateTransition(kPrefetched);
}

void PlayerIPhoneCoreAudio::playbackCB(OpaqueAudioQueue* queue, AudioQueueBuffer* buffer,
                                       bool running) {
    if (!m_queue)
        return;
    AQBuffer* audio = asBuffer(buffer);
    unsigned bytes = 0;
    unsigned packets = m_packetDescs;
    AudioFileReadPackets(m_file, 0, &bytes, audio->packets, m_packet, &packets, audio->data);
    if (packets == 0) {
        if (getLoopCount() == 0) {
            if (!running)
                return;
            Long* time = new Long(getMediaTime());
            INCREF(time);
            notifyListeners(PlayerListener::END_OF_MEDIA, time);
            DECREF(time);
            AudioQueueFlush(queue);
            AudioQueueStop(queue, 0);
            return;
        }
        if (getLoopCount() > 0)
            setLoopCount(getLoopCount() - 1);
        m_packet = 0;
        playbackCB(queue, buffer, false);
        return;
    }
    audio->byteSize = bytes;
    audio->packetCount = packets;
    m_packet += (int)packets;
    AudioQueueEnqueueBuffer(queue, buffer, 0, 0);
}

void PlayerIPhoneCoreAudio::propertyCB(OpaqueAudioQueue* queue, unsigned long property) {
    if (getState() == CLOSED || property != kQueueIsRunning)
        return;
    int running = 0;
    unsigned size = 4;
    AudioQueueGetProperty(queue, (unsigned)property, &running, &size);
    if (running == 0) {
        Player::stop();
        notifyListeners(PlayerListener::STOPPED, 0);
        return;
    }
    Long* time = new Long(getMediaTime());
    INCREF(time);
    notifyListeners(PlayerListener::STARTED, time);
    DECREF(time);
}

long long PlayerIPhoneCoreAudio::getMediaTime() const {
    double current = 0;
    unsigned char gap = 0;
    int status = AudioQueueGetCurrentTime(m_queue, m_timeline, &current, &gap);
    if (status == kInvalidRunState)
        return -1;
    unsigned size = 8;
    unsigned char rate[8];
    AudioQueueGetProperty(m_queue, kQueueSampleRate, rate, &size);
    long long sample = (long long)(current * (double)m_scale * 1000000.0);
    long long base = m_timeOffset;
    if (sample < 0)
        return base;
    return base + sample;
}

void PlayerIPhoneCoreAudio::close() {
    if (getState() == CLOSED)
        return;
    AudioLock lock;
    deallocate();
    stateTransition(CLOSED);
    if (m_queue) {
        QueueBits* info = new QueueBits;
        info->queue = m_queue;
        info->timeline = m_timeline;
        info->player = this;
        m_queue = 0;
        m_timeline = 0;
        AudioQueueCloser* closer = AudioQueueCloser::alloc()->init();
        AudioQueueStructObject* object = AudioQueueStructObject::alloc()->initWithAudioQueue_(info);
        closer->closeQueue_(object);
        closer->release();
    }
    if (m_file) {
        AudioFileClose(m_file);
        m_file = 0;
    }
}

Control* PlayerIPhoneCoreAudio::getControl(String* type) const {
    INCREF(type);
    Control* control = 0;
    if (type->equals("VolumeControl"))
        control = const_cast<VolumeControl*>(static_cast<const VolumeControl*>(this));
    DECREF(type);
    return control;
}

Class* PlayerIPhoneCoreAudio::getClass() const {
    return new PlayerIPhoneCoreAudioClass();
}

PlayerIPhoneCoreAudio::~PlayerIPhoneCoreAudio() {
#ifdef MEDECOMP_PORT
    s_pausedQueues.erase(this);
#endif
    close();
    if (m_buffers) {
        delete[] m_buffers;
        m_buffers = 0;
    }
}

void PlayerIPhoneCoreAudio::startActual() {
    if (!m_queue)
        return;
    AudioSessionSetActive(1);
    for (int i = 0; i < m_bufferCount; i++) {
        playbackCB(m_queue, m_buffers[i], false);
        Thread::sleep(0xfa);
    }
    bool failed;
    {
        AudioLock lock;
        failed = checkAudioQueueError(AudioQueuePrime(m_queue, 1, 0));
    }
    if (failed)
        return;
    Thread::sleep(0xfa);
    AudioLock lock;
    AudioQueueStart(m_queue, 0);
    AudioQueueSetParameter(m_queue, 1, (float)m_volume * 0.01f);
}

PlayerIPhoneCoreAudio::PlayerIPhoneCoreAudio(OpaqueAudioFileID* file) {
    m_file = file;
    memset(&m_format, 0, sizeof(m_format));
    m_queue = NULL;
    m_timeline = NULL;
    m_bufferCount = 3;
    m_buffers = 0;
    m_packet = 0;
    m_packetDescs = 0;
    union {
        unsigned bits;
        float value;
    } scale;
    scale.bits = kDefaultScale;
    m_scale = scale.value;
    m_duration = -1;
    m_exclusiveHeld = false;
    m_timeOffset = 0;
    m_volume = 100;
    m_buffers = new AudioQueueBuffer*[m_bufferCount];
    for (int i = 0; i < m_bufferCount; i++)
        m_buffers[i] = 0;
}

Player* PlayerIPhoneCoreAudio::duplicate() {
    return 0;
}

}
