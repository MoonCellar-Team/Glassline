#pragma once
#include "platform/AppleSDK.h"
namespace midp { class PlayerIPhoneCoreAudio; }

@interface PlayerObject : NSObject {
@public
    midp::PlayerIPhoneCoreAudio* m_player;
}
- (id)initWithPlayer:(midp::PlayerIPhoneCoreAudio*)player;
@end
