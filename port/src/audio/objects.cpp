#include "game/MediaPicker.h"
#include "platform/AudioLock.h"
#include "platform/AudioQueueTypes.h"
#include "platform/MediaPickerImpl.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/ReferenceCounted.h"
#include "port/cf.h"
#include "sdk/Foundation.h"

#include <unordered_map>

void aquireBusyLock();
void releaseLock();

AudioLock::AudioLock() { aquireBusyLock(); }

AudioLock::~AudioLock() { releaseLock(); }

template <class T>
void destroyPlain(port::cf::Object* object) {
    delete reinterpret_cast<T*>(object);
}
template <class T>
const port::cf::Class kPlainClass = {"NSObject", destroyPlain<T>, nullptr};

struct QueueBits {
    OpaqueAudioQueue* queue;
    midp::OpaqueAudioQueueTimeline* timeline;
    int count;
    AudioQueueBuffer** buffers;
    midp::PlayerIPhoneCoreAudio* player;
};

class AudioQueueAction : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<AudioQueueAction>};
    static AudioQueueAction* alloc();
    AudioQueueAction* init();
    void pauseQueue_(void* queue);
    void startQueue_(void* queue);
};

class QueueObject : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<QueueObject>};
    static QueueObject* alloc();
    QueueObject* initWithQueue_(OpaqueAudioQueue* queue);
    OpaqueAudioQueue* queue;
};

class AudioQueueStarter : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<AudioQueueStarter>};
    static AudioQueueStarter* alloc();
    AudioQueueStarter* init();
    void startPlayer_(void* player);
};

class PlayerObject : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<PlayerObject>};
    static PlayerObject* alloc();
    PlayerObject* initWithPlayer_(midp::PlayerIPhoneCoreAudio* player);
    midp::PlayerIPhoneCoreAudio* player;
};

class AudioQueueDeallocator : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<AudioQueueDeallocator>};
    static AudioQueueDeallocator* alloc();
    AudioQueueDeallocator* init();
    void deallocateQueue_(void* info);
};

class AudioQueueCloser : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<AudioQueueCloser>};
    static AudioQueueCloser* alloc();
    AudioQueueCloser* init();
    void closeQueue_(void* info);
};

class AudioQueueStructObject : public NSObject {
public:
    port::cf::Object cf{&kPlainClass<AudioQueueStructObject>};
    static AudioQueueStructObject* alloc();
    AudioQueueStructObject* initWithAudioQueue_(QueueBits* info);
    QueueBits* info;
};

namespace {

midp::ReferenceCounted* counted(midp::Player* player) {
    return static_cast<midp::ReferenceCounted*>(player);
}

std::unordered_map<MediaPickerImpl*, MediaPicker*> g_pickerManager;

}

AudioQueueAction* AudioQueueAction::alloc() { return new AudioQueueAction; }
AudioQueueAction* AudioQueueAction::init() { return this; }

void AudioQueueAction::pauseQueue_(void* object) {
    QueueObject* queue = static_cast<QueueObject*>(object);
    AudioLock lock;
    AudioQueuePause(queue->queue);
    queue->release();
}

void AudioQueueAction::startQueue_(void* object) {
    QueueObject* queue = static_cast<QueueObject*>(object);
    AudioLock lock;
    AudioQueueStart(queue->queue, 0);
    queue->release();
}

QueueObject* QueueObject::alloc() { return new QueueObject; }

QueueObject* QueueObject::initWithQueue_(OpaqueAudioQueue* queue) {
    this->queue = queue;
    return this;
}

AudioQueueStarter* AudioQueueStarter::alloc() { return new AudioQueueStarter; }
AudioQueueStarter* AudioQueueStarter::init() { return this; }

void AudioQueueStarter::startPlayer_(void* object) {
    PlayerObject* holder = static_cast<PlayerObject*>(object);
    holder->player->startActual();
    midp::DECREF(counted(holder->player));
    holder->release();
}

PlayerObject* PlayerObject::alloc() { return new PlayerObject; }

PlayerObject* PlayerObject::initWithPlayer_(midp::PlayerIPhoneCoreAudio* player) {
    this->player = player;
    midp::INCREF(counted(player));
    return this;
}

AudioQueueDeallocator* AudioQueueDeallocator::alloc() { return new AudioQueueDeallocator; }
AudioQueueDeallocator* AudioQueueDeallocator::init() { return this; }

void AudioQueueDeallocator::deallocateQueue_(void* object) {
    AudioQueueStructObject* holder = static_cast<AudioQueueStructObject*>(object);
    QueueBits* data = holder->info;
    {
        AudioLock lock;
        AudioQueueStop(data->queue, 1);
    }
    for (int i = 0; i < data->count; i++) {
        if (data->buffers[i] != 0)
            AudioQueueFreeBuffer(data->queue, data->buffers[i]);
    }
    delete[] data->buffers;
    {
        AudioLock lock;
        AudioQueueRemovePropertyListener(data->queue, 0x6171726e, midp::AudioQueue_property, data->player);
    }
    delete data;
    holder->release();
}

AudioQueueCloser* AudioQueueCloser::alloc() { return new AudioQueueCloser; }
AudioQueueCloser* AudioQueueCloser::init() { return this; }

void AudioQueueCloser::closeQueue_(void* object) {
    AudioQueueStructObject* holder = static_cast<AudioQueueStructObject*>(object);
    QueueBits* data = holder->info;
    {
        AudioLock lock;
        midp::Player* player = data->player;
        player->notifyListeners(midp::PlayerListener::CLOSED, 0);
        midp::DECREF(counted(player));
        AudioQueueDisposeTimeline(data->queue, reinterpret_cast<OpaqueAudioQueueTimeline*>(data->timeline));
        AudioQueueDispose(data->queue, 1);
    }
    delete data;
    holder->release();
}

AudioQueueStructObject* AudioQueueStructObject::alloc() { return new AudioQueueStructObject; }

AudioQueueStructObject* AudioQueueStructObject::initWithAudioQueue_(QueueBits* info) {
    this->info = info;
    return this;
}

MediaPickerImpl* MediaPickerImpl::alloc() { return new MediaPickerImpl; }

MediaPickerImpl* MediaPickerImpl::initWithManager(MediaPicker* manager) {
    g_pickerManager[this] = manager;
    return this;
}
