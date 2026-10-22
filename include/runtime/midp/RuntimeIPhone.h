#pragma once

#include "runtime/midp/Runtime.h"

namespace midp {

class String;

class RuntimeIPhone : public Runtime {
public:
    bool isOSMediaPickerSupported();
    void suspendAudio();
    void resumeAudio();
    static void launchBrowser(String* url);
    void* getPushOption(void* key);
    RuntimeIPhone();
    virtual ~RuntimeIPhone();
    bool isOtherAudioPlaying();
    void* getDeviceToken() const;
    void setDeviceToken(void* token);
    void setPushStartup(void* payload);
    bool isPushStartup() const;
    void* getPushOption(const void* key);
    void applicationDidReceiveRemoteNotification(void* userInfo);
    void applicationWillTerminate(void*);
    void applicationDidReceiveMemoryWarning(void*);
    void applicationDidFinishLaunching(void*);
    virtual void scheduleTimerAtFixedRate(util::Timer* timer, long delay, long period);
    virtual void scheduleTimer(util::Timer* timer, long delay, long period);
    virtual void scheduleTimer(util::Timer* timer, long delay);
    virtual bool eventLoop();
    String* getOSVersion();
    void applicationDidBecomeActive(void*);
    void applicationWillResignActive(void*);
    int totalMemory();
    int freeMemory();

    int m_unk78;
    void* m_deviceToken;
    bool m_pushStartup;
    void* m_platformType;
    bool m_audioSuspended;
    int m_otherAudio;
    void* m_pushOptions;
};

}
