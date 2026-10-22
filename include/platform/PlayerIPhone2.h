#pragma once
#include "platform/AppleSDK.h"

namespace midp { class PlayerIPhoneVideo; }

@interface PlayerIPhone2 : NSObject {
@public
    midp::PlayerIPhoneVideo* m_playeriphone;
    NSURL* resourceURL;
    MPMoviePlayerController* mMoviePlayer;
}
- (id)initWithCppInterface:(NSURL*)url :(midp::PlayerIPhoneVideo*)player;
@end
