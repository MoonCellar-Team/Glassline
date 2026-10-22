#include "decode.h"
#include "openal.h"
#include "platform/AudioQueueTypes.h"
#include "port/files.h"
#include "sdk/AudioToolbox.h"
#include "sdk/CoreFoundation.h"

#include <atomic>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

const int kNoErr = 0;
const int kInvalid = -0x10476;
const unsigned kFileFormat = 0x66666d74;
const unsigned kDataFormat = 0x64666d74;
const unsigned kByteCount = 0x62636e74;
const unsigned kPacketCount = 0x70636e74;
const unsigned kPacketUpper = 0x706b7562;
const unsigned kMagicCookie = 0x6d676963;
const unsigned kChannelLayout = 0x636d6170;
const unsigned kIsRunning = 0x6171726e;
const unsigned kSampleRate = 0x61717372;
const unsigned kWave = 0x57415645;
const unsigned kCaff = 0x63616666;
const unsigned kLpcm = 0x6c70636d;
const int kAlcFrequency = 0x1007;
const int kAlGain = 0x100A;
const int kAlBuffer = 0x1009;
const int kAlSampleOffset = 0x1025;
const int kAlBuffersProcessed = 0x1016;
const int kAlSourceState = 0x1010;
const int kAlPlaying = 0x1012;
const int kAlBuffersQueued = 0x1015;
const int kAlFormatMono8 = 0x1100;
const int kAlFormatMono16 = 0x1101;
const int kAlFormatStereo8 = 0x1102;
const int kAlFormatStereo16 = 0x1103;

struct AqBuf {
    unsigned capacity;
    void* data;
    unsigned byteSize;
    void* user;
    unsigned packetCapacity;
    void* packets;
    unsigned packetCount;
    unsigned alName;
    int pending;
};

struct File {
    FILE* handle;
    AudioStreamBasicDescription format;
    unsigned long long dataPos;
    unsigned long long dataBytes;
    unsigned fourcc;
};

using Callback = void (*)(void*, OpaqueAudioQueue*, AudioQueueBuffer*);
using PropCallback = void (*)(void*, OpaqueAudioQueue*, unsigned long);

struct Queue {
    AudioStreamBasicDescription format;
    Callback callback;
    void* user;
    void* device;
    void* context;
    unsigned source;
    float volume;
    std::atomic<int> running;
    std::atomic<int> wantPlay;
    std::atomic<int> quit;
    std::atomic<int> everStarted;
    std::atomic<long long> frames;
    long long played;
    std::vector<AqBuf*> buffers;
    std::vector<AqBuf*> order;
    std::vector<PropCallback> props;
    std::vector<void*> propUsers;
    std::mutex mu;
    std::thread thread;
};

}

extern "C" {
void* alcOpenDevice(const char*);
void* alcCreateContext(void*, const int*);
void alcMakeContextCurrent(void*);
void* alcGetCurrentContext();
void* alcGetProcAddress(void*, const char*);
void alcDestroyContext(void*);
int alcCloseDevice(void*);
void alGenSources(int, unsigned*);
void alDeleteSources(int, const unsigned*);
void alGenBuffers(int, unsigned*);
void alDeleteBuffers(int, const unsigned*);
void alBufferData(unsigned, int, const void*, int, int);
void alSourceQueueBuffers(unsigned, int, const unsigned*);
void alSourceUnqueueBuffers(unsigned, int, unsigned*);
void alSourcePlay(unsigned);
void alSourcePause(unsigned);
void alSourceStop(unsigned);
void alSourcef(unsigned, int, float);
void alSourcei(unsigned, int, int);
void alGetSourcei(unsigned, int, int*);
int alGetError();
}

namespace {

int alFormat(const AudioStreamBasicDescription& f) {
    if (f.mChannelsPerFrame == 1)
        return f.mBitsPerChannel == 8 ? kAlFormatMono8 : kAlFormatMono16;
    return f.mBitsPerChannel == 8 ? kAlFormatStereo8 : kAlFormatStereo16;
}

unsigned frameBytes(const AudioStreamBasicDescription& f) {
    if (f.mBytesPerFrame != 0)
        return f.mBytesPerFrame;
    unsigned channels = f.mChannelsPerFrame == 0 ? 1 : f.mChannelsPerFrame;
    unsigned bits = f.mBitsPerChannel == 0 ? 16 : f.mBitsPerChannel;
    return channels * bits / 8;
}

using SetThreadContext = int (*)(void*);
using GetThreadContext = void* (*)();
SetThreadContext setThreadContext() {
    static SetThreadContext fn = reinterpret_cast<SetThreadContext>(alcGetProcAddress(0, "alcSetThreadContext"));
    return fn;
}
GetThreadContext getThreadContext() {
    static GetThreadContext fn = reinterpret_cast<GetThreadContext>(alcGetProcAddress(0, "alcGetThreadContext"));
    return fn;
}

struct AlUse {
    void* previous;
    bool local;
    AlUse(Queue* q) : previous(0), local(setThreadContext() != 0 && getThreadContext() != 0) {
        if (local) {
            previous = getThreadContext()();
            setThreadContext()(q->context);
        } else {
            previous = alcGetCurrentContext();
            alcMakeContextCurrent(q->context);
        }
    }
    ~AlUse() {
        if (local)
            setThreadContext()(previous);
        else
            alcMakeContextCurrent(previous);
    }
};

void ensureDevice(Queue* q) {
    if (q->context != 0)
        return;
    q->device = alcOpenDevice(0);
    int rate = (int)q->format.mSampleRate;
    const int attrs[3] = {kAlcFrequency, rate > 0 ? rate : 22050, 0};
    q->context = alcCreateContext(q->device, attrs);
    AlUse use(q);
    alGenSources(1, &q->source);
    alSourcef(q->source, kAlGain, q->volume);
    alGetError();
}

void uploadPending(Queue* q) {
    std::vector<AqBuf*> order;
    order.swap(q->order);
    for (AqBuf* buffer : order) {
        if (!buffer->pending || buffer->data == 0)
            continue;
        buffer->pending = 0;
        if (buffer->alName != 0)
            alDeleteBuffers(1, &buffer->alName);
        alGenBuffers(1, &buffer->alName);
        alBufferData(buffer->alName, alFormat(q->format), buffer->data, (int)buffer->byteSize,
                     (int)q->format.mSampleRate);
        alSourceQueueBuffers(q->source, 1, &buffer->alName);
    }
    alGetError();
}

void recycle(Queue* q, std::vector<AqBuf*>* done) {
    if (q->source == 0)
        return;
    int processed = 0;
    alGetSourcei(q->source, kAlBuffersProcessed, &processed);
    unsigned frame = frameBytes(q->format);
    while (processed > 0) {
        unsigned name = 0;
        alSourceUnqueueBuffers(q->source, 1, &name);
        processed--;
        for (AqBuf* buffer : q->buffers) {
            if (buffer->alName == name) {
                q->played += frame == 0 ? 0 : buffer->byteSize / frame;
                alDeleteBuffers(1, &buffer->alName);
                buffer->alName = 0;
                buffer->byteSize = 0;
                done->push_back(buffer);
                break;
            }
        }
    }
    int offset = 0;
    alGetSourcei(q->source, kAlSampleOffset, &offset);
    q->frames.store(q->played + offset);
}

void notifyRunning(Queue* q, int running) {
    std::vector<PropCallback> fns = q->props;
    std::vector<void*> users = q->propUsers;
    for (size_t i = 0; i < fns.size(); i++)
        fns[i](users[i], reinterpret_cast<OpaqueAudioQueue*>(q), kIsRunning);
    (void)running;
}

void worker(Queue* q) {
    int logTicks = 0;
    while (!q->quit.load()) {
        if (port::audio::logOn() && q->running.load() && ++logTicks % 133 == 0)
            port::audio::log("очередь %p: играет, громкость %.2f, позиция %lld сэмплов", (void*)q, q->volume,
                             (long long)q->frames.load());
        std::vector<AqBuf*> done;
        int tell = 0;
        {
            std::lock_guard<std::mutex> lock(q->mu);
            if (q->context == 0 && (q->wantPlay.load() != 0 || !q->buffers.empty()))
                ensureDevice(q);
            if (q->context != 0) {
                AlUse use(q);
                uploadPending(q);
                int want = q->wantPlay.load();
                alSourcef(q->source, kAlGain, q->volume);
                if (want == 1) {
                    int state = 0, queued = 0;
                    alGetSourcei(q->source, kAlSourceState, &state);
                    alGetSourcei(q->source, kAlBuffersQueued, &queued);
                    if (state != kAlPlaying && queued > 0) {
                        if (q->running.load() && port::audio::logOn())
                            port::audio::log("очередь %p: опустела, звук прерывался (позиция %lld)", (void*)q,
                                             (long long)q->frames.load());
                        alSourcePlay(q->source);
                    }
                    if (!q->running.load()) {
                        q->running.store(1);
                        q->everStarted.store(1);
                        tell = 1;
                    }
                } else if (want == 2) {
                    alSourcePause(q->source);
                } else if (q->running.load()) {
                    alSourceStop(q->source);
                    q->running.store(0);
                    tell = 1;
                }
                recycle(q, &done);
            }
        }
        if (tell)
            notifyRunning(q, q->running.load());
        for (AqBuf* buffer : done) {
            if (q->callback != 0)
                q->callback(q->user, reinterpret_cast<OpaqueAudioQueue*>(q),
                            reinterpret_cast<AudioQueueBuffer*>(buffer));
        }
        usleep(15000);
    }
    std::lock_guard<std::mutex> lock(q->mu);
    if (q->context != 0) {
        {
            AlUse use(q);
            if (q->source != 0) {
                alSourceStop(q->source);
                alSourcei(q->source, kAlBuffer, 0);
                alDeleteSources(1, &q->source);
            }
            for (AqBuf* buffer : q->buffers) {
                if (buffer->alName != 0)
                    alDeleteBuffers(1, &buffer->alName);
            }
        }
        alcDestroyContext(q->context);
        alcCloseDevice(q->device);
        q->context = 0;
        q->device = 0;
        q->source = 0;
    }
}

unsigned readLe32(FILE* file) {
    unsigned char b[4];
    if (fread(b, 1, 4, file) != 4)
        return 0;
    return (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
}

unsigned readLe16(FILE* file) {
    unsigned char b[2];
    if (fread(b, 1, 2, file) != 2)
        return 0;
    return (unsigned)b[0] | ((unsigned)b[1] << 8);
}

bool parseWav(File* file) {
    char riff[4];
    if (fread(riff, 1, 4, file->handle) != 4 || memcmp(riff, "RIFF", 4) != 0)
        return false;
    readLe32(file->handle);
    char wave[4];
    if (fread(wave, 1, 4, file->handle) != 4 || memcmp(wave, "WAVE", 4) != 0)
        return false;
    bool haveFmt = false;
    while (1) {
        char id[4];
        if (fread(id, 1, 4, file->handle) != 4)
            return false;
        unsigned size = readLe32(file->handle);
        long next = ftell(file->handle) + (long)size + (size & 1);
        if (memcmp(id, "fmt ", 4) == 0 && size >= 16) {
            unsigned format = readLe16(file->handle);
            unsigned channels = readLe16(file->handle);
            unsigned rate = readLe32(file->handle);
            readLe32(file->handle);
            unsigned block = readLe16(file->handle);
            unsigned bits = readLe16(file->handle);
            if (format != 1)
                return false;
            file->format.mSampleRate = (double)rate;
            file->format.mFormatID = kLpcm;
            file->format.mFormatFlags = bits == 8 ? 8 : 12;
            file->format.mBytesPerPacket = block;
            file->format.mFramesPerPacket = 1;
            file->format.mBytesPerFrame = block;
            file->format.mChannelsPerFrame = channels;
            file->format.mBitsPerChannel = bits;
            haveFmt = true;
        } else if (memcmp(id, "data", 4) == 0) {
            file->dataPos = (unsigned long long)ftell(file->handle);
            file->dataBytes = size;
            file->fourcc = kWave;
            return haveFmt;
        }
        if (fseek(file->handle, next, SEEK_SET) != 0)
            return false;
    }
}

bool decodeFile(File* file, const char* path) {
    std::vector<unsigned char> data, pcm;
    unsigned char chunk[65536];
    rewind(file->handle);
    size_t n;
    while ((n = fread(chunk, 1, sizeof chunk, file->handle)) > 0)
        data.insert(data.end(), chunk, chunk + n);
    int rate = 0, channels = 0;
    if (!port::audio::decodeToPcm16(data, &pcm, &rate, &channels)) {
        fprintf(stderr, "medecomp: звук не декодируется (нет FFmpeg или файл повреждён): %s\n", path);
        rate = 22050;
        channels = 2;
        pcm.assign((size_t)rate * 4, 0);
    }
    FILE* memory = port::files::memoryFile(pcm);
    if (memory == 0)
        return false;
    fclose(file->handle);
    file->handle = memory;
    unsigned block = (unsigned)channels * 2;
    file->format.mSampleRate = (double)rate;
    file->format.mFormatID = kLpcm;
    file->format.mFormatFlags = 12;
    file->format.mBytesPerPacket = block;
    file->format.mFramesPerPacket = 1;
    file->format.mBytesPerFrame = block;
    file->format.mChannelsPerFrame = (unsigned)channels;
    file->format.mBitsPerChannel = 16;
    file->dataPos = 0;
    file->dataBytes = pcm.size();
    file->fourcc = kCaff;
    if (port::audio::logOn())
        port::audio::log("звук: %s — %d Гц, каналов %d, %.1f с", path, rate, channels, (double)pcm.size() / block / rate);
    return true;
}

File* asFile(OpaqueAudioFileID* file) { return reinterpret_cast<File*>(file); }
Queue* asQueue(OpaqueAudioQueue* queue) { return reinterpret_cast<Queue*>(queue); }

}

namespace midp {
struct OpaqueAudioQueueTimeline;
}

extern "C" {

int AudioFileOpenURL(const __CFURL* url, int, unsigned int, OpaqueAudioFileID** outFile) {
    if (outFile == 0)
        return kInvalid;
    *outFile = 0;
    char path[1024];
    CFURLRef ref = reinterpret_cast<CFURLRef>(const_cast<struct __CFURL*>(url));
    if (url == 0 || !CFURLGetFileSystemRepresentation(ref, true, path, sizeof path))
        return kInvalid;
    FILE* handle = port::files::open(path);
    if (handle == 0)
        return kInvalid;
    File* file = new File();
    memset(file, 0, sizeof *file);
    file->handle = handle;
    if (!parseWav(file) && !decodeFile(file, path)) {
        fclose(file->handle);
        delete file;
        return kInvalid;
    }
    *outFile = reinterpret_cast<OpaqueAudioFileID*>(file);
    return kNoErr;
}

int AudioFileClose(OpaqueAudioFileID* id) {
    File* file = asFile(id);
    if (file == 0)
        return kInvalid;
    if (file->handle != 0)
        fclose(file->handle);
    delete file;
    return kNoErr;
}

int AudioFileGetPropertyInfo(OpaqueAudioFileID* id, unsigned property, unsigned* size, unsigned* writable) {
    File* file = asFile(id);
    if (file == 0)
        return kInvalid;
    unsigned bytes = 0;
    if (property == kDataFormat)
        bytes = 0x28;
    else if (property == kFileFormat || property == kPacketUpper)
        bytes = 4;
    else if (property == kByteCount || property == kPacketCount)
        bytes = 8;
    else if (property == kMagicCookie || property == kChannelLayout)
        return kInvalid;
    else
        return kInvalid;
    if (size != 0)
        *size = bytes;
    if (writable != 0)
        *writable = 0;
    return kNoErr;
}

int AudioFileGetProperty(OpaqueAudioFileID* id, unsigned property, unsigned* size, void* data) {
    File* file = asFile(id);
    if (file == 0 || data == 0)
        return kInvalid;
    if (property == kDataFormat) {
        memcpy(data, &file->format, sizeof file->format);
        if (size != 0)
            *size = 0x28;
        return kNoErr;
    }
    if (property == kFileFormat) {
        memcpy(data, &file->fourcc, 4);
        if (size != 0)
            *size = 4;
        return kNoErr;
    }
    if (property == kByteCount) {
        memcpy(data, &file->dataBytes, 8);
        if (size != 0)
            *size = 8;
        return kNoErr;
    }
    if (property == kPacketCount) {
        unsigned long long packets = 0;
        if (file->format.mBytesPerPacket != 0)
            packets = file->dataBytes / file->format.mBytesPerPacket;
        memcpy(data, &packets, 8);
        if (size != 0)
            *size = 8;
        return kNoErr;
    }
    if (property == kPacketUpper) {
        unsigned upper = file->format.mBytesPerPacket;
        memcpy(data, &upper, 4);
        if (size != 0)
            *size = 4;
        return kNoErr;
    }
    return kInvalid;
}

int AudioFileReadBytes(OpaqueAudioFileID* id, unsigned char, long long start, unsigned* count, void* buffer) {
    File* file = asFile(id);
    if (file == 0 || count == 0 || buffer == 0)
        return kInvalid;
    if (start < 0 || (unsigned long long)start >= file->dataBytes) {
        *count = 0;
        return kNoErr;
    }
    unsigned long long rest = file->dataBytes - (unsigned long long)start;
    unsigned n = *count;
    if ((unsigned long long)n > rest)
        n = (unsigned)rest;
    if (fseek(file->handle, (long)(file->dataPos + (unsigned long long)start), SEEK_SET) != 0)
        return kInvalid;
    n = (unsigned)fread(buffer, 1, n, file->handle);
    *count = n;
    return kNoErr;
}

int AudioFileReadPackets(OpaqueAudioFileID* id, int, unsigned* outBytes, void*, long long startPacket,
                         unsigned* ioPackets, void* buffer) {
    File* file = asFile(id);
    if (file == 0 || ioPackets == 0 || buffer == 0)
        return kInvalid;
    unsigned frame = frameBytes(file->format);
    if (frame == 0)
        return kInvalid;
    unsigned packets = *ioPackets;
    unsigned count = packets * frame;
    int status = AudioFileReadBytes(id, 0, startPacket * (long long)frame, &count, buffer);
    if (status != kNoErr)
        return status;
    *ioPackets = frame == 0 ? 0 : count / frame;
    if (outBytes != 0)
        *outBytes = count;
    return kNoErr;
}

int AudioQueueNewOutput(const AudioStreamBasicDescription* format, void (*callback)(), void* user,
                        const void*, const void*, unsigned, OpaqueAudioQueue** outQueue) {
    if (outQueue == 0 || format == 0)
        return kInvalid;
    Queue* queue = new Queue();
    memset(&queue->format, 0, sizeof queue->format);
    queue->format = *format;
    queue->callback = reinterpret_cast<Callback>(callback);
    queue->user = user;
    queue->device = 0;
    queue->context = 0;
    queue->source = 0;
    queue->volume = 1;
    queue->running.store(0);
    queue->wantPlay.store(0);
    queue->quit.store(0);
    queue->everStarted.store(0);
    queue->frames.store(0);
    queue->played = 0;
    queue->thread = std::thread(worker, queue);
    *outQueue = reinterpret_cast<OpaqueAudioQueue*>(queue);
    return kNoErr;
}

int AudioQueueDispose(OpaqueAudioQueue* id, BOOL) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    queue->quit.store(1);
    if (queue->thread.joinable())
        queue->thread.join();
    delete queue;
    return kNoErr;
}

int AudioQueueStart(OpaqueAudioQueue* id, const void*) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    queue->wantPlay.store(1);
    return kNoErr;
}

int AudioQueuePause(OpaqueAudioQueue* id) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    queue->wantPlay.store(2);
    return kNoErr;
}

int AudioQueueStop(OpaqueAudioQueue* id, BOOL) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    queue->wantPlay.store(0);
    if (queue->thread.get_id() == std::this_thread::get_id())
        return kNoErr;
    for (int i = 0; i < 200 && queue->running.load(); i++)
        usleep(1000);
    return kNoErr;
}

int AudioQueueFlush(OpaqueAudioQueue* id) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    std::lock_guard<std::mutex> lock(queue->mu);
    queue->frames.store(0);
    queue->played = 0;
    queue->order.clear();
    for (AqBuf* buffer : queue->buffers)
        buffer->pending = 0;
    if (queue->context != 0) {
        AlUse use(queue);
        alSourceStop(queue->source);
        alSourcei(queue->source, kAlBuffer, 0);
        for (AqBuf* buffer : queue->buffers) {
            if (buffer->alName != 0) {
                alDeleteBuffers(1, &buffer->alName);
                buffer->alName = 0;
            }
        }
    }
    return kNoErr;
}

int AudioQueuePrime(OpaqueAudioQueue*, unsigned, unsigned* prepared) {
    if (prepared != 0)
        *prepared = 0;
    return kNoErr;
}

int AudioQueueCreateTimeline(OpaqueAudioQueue*, midp::OpaqueAudioQueueTimeline** out) {
    if (out == 0)
        return kInvalid;
    *out = reinterpret_cast<midp::OpaqueAudioQueueTimeline*>(malloc(1));
    return *out == 0 ? kInvalid : kNoErr;
}

int AudioQueueDisposeTimeline(OpaqueAudioQueue*, OpaqueAudioQueueTimeline* timeline) {
    free(timeline);
    return kNoErr;
}

int AudioQueueGetCurrentTime(OpaqueAudioQueue* id, midp::OpaqueAudioQueueTimeline*, double* time,
                             unsigned char* gap) {
    Queue* queue = asQueue(id);
    if (queue == 0 || !queue->everStarted.load())
        return kInvalid;
    if (time != 0)
        *time = (double)queue->frames.load();
    if (gap != 0)
        *gap = 0;
    return kNoErr;
}

int AudioQueueGetProperty(OpaqueAudioQueue* id, unsigned property, void* data, unsigned* size) {
    Queue* queue = asQueue(id);
    if (queue == 0 || data == 0)
        return kInvalid;
    if (property == kIsRunning) {
        int running = queue->running.load();
        memcpy(data, &running, 4);
        if (size != 0)
            *size = 4;
        return kNoErr;
    }
    if (property == kSampleRate) {
        double rate = queue->format.mSampleRate;
        memcpy(data, &rate, 8);
        if (size != 0)
            *size = 8;
        return kNoErr;
    }
    return kInvalid;
}

int AudioQueueSetProperty(OpaqueAudioQueue*, unsigned, const void*, unsigned) { return kNoErr; }

int AudioQueueAddPropertyListener(OpaqueAudioQueue* id, unsigned, void (*callback)(), void* user) {
    Queue* queue = asQueue(id);
    if (queue == 0 || callback == 0)
        return kInvalid;
    std::lock_guard<std::mutex> lock(queue->mu);
    queue->props.push_back(reinterpret_cast<PropCallback>(callback));
    queue->propUsers.push_back(user);
    return kNoErr;
}

int AudioQueueRemovePropertyListener(OpaqueAudioQueue* id, unsigned long, void (*callback)(void*, OpaqueAudioQueue*, unsigned long),
                                     void* user) {
    Queue* queue = asQueue(id);
    if (queue == 0)
        return kInvalid;
    std::lock_guard<std::mutex> lock(queue->mu);
    for (size_t i = 0; i < queue->props.size(); i++) {
        if (queue->props[i] == callback && queue->propUsers[i] == user) {
            queue->props.erase(queue->props.begin() + (long)i);
            queue->propUsers.erase(queue->propUsers.begin() + (long)i);
            break;
        }
    }
    return kNoErr;
}

int AudioQueueAllocateBufferWithPacketDescriptions(OpaqueAudioQueue* id, unsigned bytes, unsigned packets,
                                                   AudioQueueBuffer** out) {
    Queue* queue = asQueue(id);
    if (queue == 0 || out == 0)
        return kInvalid;
    AqBuf* buffer = (AqBuf*)calloc(1, sizeof(AqBuf));
    buffer->capacity = bytes;
    buffer->data = calloc(1, bytes == 0 ? 1 : bytes);
    buffer->packetCapacity = packets;
    if (packets != 0)
        buffer->packets = calloc(packets, 16);
    std::lock_guard<std::mutex> lock(queue->mu);
    queue->buffers.push_back(buffer);
    *out = reinterpret_cast<AudioQueueBuffer*>(buffer);
    return kNoErr;
}

int AudioQueueEnqueueBuffer(OpaqueAudioQueue* id, AudioQueueBuffer* buffer, unsigned, const void*) {
    Queue* queue = asQueue(id);
    AqBuf* item = reinterpret_cast<AqBuf*>(buffer);
    if (queue == 0 || item == 0)
        return kInvalid;
    std::lock_guard<std::mutex> lock(queue->mu);
    item->pending = 1;
    queue->order.push_back(item);
    return kNoErr;
}

int AudioQueueFreeBuffer(OpaqueAudioQueue* id, AudioQueueBuffer* buffer) {
    Queue* queue = asQueue(id);
    AqBuf* item = reinterpret_cast<AqBuf*>(buffer);
    if (queue == 0 || item == 0)
        return kInvalid;
    std::lock_guard<std::mutex> lock(queue->mu);
    if (queue->context != 0 && item->alName != 0) {
        AlUse use(queue);
        alDeleteBuffers(1, &item->alName);
    }
    for (size_t i = 0; i < queue->buffers.size(); i++) {
        if (queue->buffers[i] == item) {
            queue->buffers.erase(queue->buffers.begin() + (long)i);
            break;
        }
    }
    for (size_t i = 0; i < queue->order.size();) {
        if (queue->order[i] == item)
            queue->order.erase(queue->order.begin() + (long)i);
        else
            i++;
    }
    free(item->data);
    free(item->packets);
    free(item);
    return kNoErr;
}

int AudioQueueGetParameter(OpaqueAudioQueue* id, unsigned param, float* value) {
    Queue* queue = asQueue(id);
    if (queue == 0 || value == 0 || param != 1)
        return kInvalid;
    *value = queue->volume;
    return kNoErr;
}

int AudioQueueSetParameter(OpaqueAudioQueue* id, unsigned param, float value) {
    Queue* queue = asQueue(id);
    if (queue == 0 || param != 1)
        return kInvalid;
    queue->volume = value;
    return kNoErr;
}

int AudioSessionGetProperty(unsigned, unsigned* size, void* value) {
    if (value != 0 && size != 0 && *size >= 4)
        memset(value, 0, 4);
    return kNoErr;
}

int AudioSessionSetProperty(unsigned, unsigned, const void*) { return kNoErr; }

int AudioSessionSetActive(unsigned active) {
    port::audio::setSessionActive(active != 0);
    return kNoErr;
}

}
