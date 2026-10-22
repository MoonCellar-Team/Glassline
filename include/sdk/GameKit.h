#pragma once

#include "sdk/Foundation.h"

class NSError;

class GKLocalPlayer : public NSObject {
public:
    static GKLocalPlayer* localPlayer();
    void authenticateWithCompletionHandler(void (^completionHandler)(NSError* error));
    bool isAuthenticated();
};
