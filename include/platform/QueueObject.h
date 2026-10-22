#pragma once
#include "platform/AppleSDK.h"
#include "sdk/AudioToolbox.h"

@interface QueueObject : NSObject {
@public
    OpaqueAudioQueue* m_queue;
}
- (id)initWithQueue:(OpaqueAudioQueue*)queue;
@end
