#pragma once
#include "platform/AppleSDK.h"
#include "platform/AudioQueueTypes.h"

@interface AudioQueueStructObject : NSObject {
@public
    AudioQueue_posixStruct* m_audioQueue;
}
- (id)initWithAudioQueue:(AudioQueue_posixStruct*)queue;
@end
