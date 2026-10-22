#pragma once
#include "sdk/AudioToolbox.h"
namespace midp { class PlayerIPhoneCoreAudio; }
struct OpaqueAudioQueueTimeline;
struct AudioQueue_posixStruct {
    OpaqueAudioQueue* queue;
    OpaqueAudioQueueTimeline* timeline;
    int count;
    AudioQueueBuffer** buffers;
    midp::PlayerIPhoneCoreAudio* player;
};
extern "C" {
typedef signed char BOOL;
int AudioQueuePause(OpaqueAudioQueue*);
int AudioQueueStart(OpaqueAudioQueue*,const void*);
int AudioQueueStop(OpaqueAudioQueue*,BOOL);
int AudioQueueFreeBuffer(OpaqueAudioQueue*,AudioQueueBuffer*);
int AudioQueueRemovePropertyListener(OpaqueAudioQueue*,unsigned long,void (*)(void*,OpaqueAudioQueue*,unsigned long),void*);
int AudioQueueDisposeTimeline(OpaqueAudioQueue*,OpaqueAudioQueueTimeline*);
int AudioQueueDispose(OpaqueAudioQueue*,BOOL);
}
