#include "runtime/midp/RuntimeIPhone.h"
#include <stdint.h>

#include "game/MonkeyApp.h"
#include "platform/AudioLock.h"
#include "runtime/midp/AccelerometerIPhone.h"
#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/MIDlet.h"
#include "runtime/midp/PlayerIPhoneOpenAL.h"
#include "runtime/midp/RuntimeTimerTask.h"
#include "runtime/midp/String.h"
#include "runtime/util/Timer.h"
#include "sdk/CoreFoundation.h"
#include "sdk/UIKit.h"

#include <map>
#include <string>

#ifdef MEDECOMP_PORT
extern "C" int medecomp_background_audio(void);
#endif

class CocoaTimer : public NSObject {
public:
    static CocoaTimer* alloc();
    CocoaTimer* init();
    void schedule_withDelay_andPeriod_(util::Timer* timer, long delay, long period);
    void scheduleOnce_withDelay_(util::Timer* timer, long delay);
};

class MPMusicPlayerController : public NSObject {
public:
    static MPMusicPlayerController* iPodMusicPlayer();
    int playbackState();
};

namespace {

const unsigned kAudioCategory = 0x61636174;
const unsigned kCategorySolo = 0x736f6c6f;
const unsigned kCategoryAmbient = 0x616d6269;
const unsigned kHardwareSampleRate = 0x68777372;
const unsigned kOtherAudioPlaying = 0x6f746872;
const unsigned kSampleRateHi = 0x40d58880;

extern "C" int AudioSessionGetProperty(unsigned property, unsigned* size, void* value);
extern "C" int AudioSessionSetProperty(unsigned property, unsigned size, const void* value);
extern "C" int AudioSessionSetActive(unsigned active);
extern "C" void* alcGetCurrentContext();
extern "C" void alcMakeContextCurrent(void* context);
extern "C" void alcProcessContext(void* context);
extern "C" void alcSuspendContext(void* context);
extern "C" int puts(const char* text);
extern "C" void perror(const char* text);
extern "C" int sysctl(int* name, unsigned count, void* oldp, unsigned* oldlen, void* newp,
                      unsigned newlen);
extern "C" void NSLog(const void* format, ...);
extern "C" void CleanIPSP(void* leftover);
extern "C" void* objc_msgSend(void* object, const char* selector, ...);
extern "C" void objc_enumerationMutation(void* object);

struct FastEnumState {
    unsigned state;
    void** items;
    unsigned* mutations;
    unsigned extra[5];
};

int otherAudioState() {
    unsigned size = 4;
    int playing = 0;
    AudioSessionGetProperty(kOtherAudioPlaying, &size, &playing);
    return playing == 0 ? 2 : 1;
}

}

MonkeyApp* implementation_getMIDlet();

namespace midp {

void* RuntimeIPhone::getDeviceToken() const {
    return m_deviceToken;
}

void RuntimeIPhone::setPushStartup(void* payload) {
    m_pushStartup = true;
    m_pushOptions = payload;
}

bool RuntimeIPhone::isPushStartup() const {
    return m_pushStartup;
}

void RuntimeIPhone::launchBrowser(String* url) {
    UIApplication* app = UIApplication::sharedApplication();
    NSURL* link = NSURL::URLWithString(
        reinterpret_cast<NSString*>(const_cast<__CFString*>(url->getCFString())));
    app->openURL(link);
}

void* RuntimeIPhone::getPushOption(const void* key) {
    if (!m_pushOptions)
        return 0;
    return static_cast<NSDictionary*>(m_pushOptions)
        ->objectForKey(reinterpret_cast<NSString*>(const_cast<void*>(key)));
}

void RuntimeIPhone::setDeviceToken(void* token) {
    if (token)
        static_cast<NSObject*>(token)->retain();
    if (m_deviceToken)
        static_cast<NSObject*>(m_deviceToken)->release();
    m_deviceToken = token;
}

void RuntimeIPhone::applicationDidReceiveRemoteNotification(void* userInfo) {
    std::map<std::string, std::string> info;
    FastEnumState state = {};
    void* objects[16];
    unsigned count = (unsigned)reinterpret_cast<uintptr_t>(objc_msgSend(
        userInfo, "countByEnumeratingWithState:objects:count:", &state, objects, 0x10));
    while (count != 0) {
        unsigned saved = *state.mutations;
        for (unsigned i = 0; i < count; i++) {
            if (*state.mutations != saved)
                objc_enumerationMutation(userInfo);
            NSString* key = static_cast<NSString*>(state.items[i]);
            if (key->compare(reinterpret_cast<NSString*>(
                    const_cast<__CFString*>(CFSTR("aps")))) == 0) {
                void* aps = static_cast<NSDictionary*>(userInfo)->objectForKey(key);
                NSString* alert = static_cast<NSString*>(
                    static_cast<NSDictionary*>(aps)->objectForKey(
                        reinterpret_cast<NSString*>(const_cast<__CFString*>(CFSTR("alert")))));
                info["alert"] = alert->cStringUsingEncoding(NSString::defaultCStringEncoding());
            } else {
                void* value = static_cast<NSDictionary*>(userInfo)->objectForKey(key);
                const char* name = key->cStringUsingEncoding(NSString::defaultCStringEncoding());
                const char* text = static_cast<NSString*>(value)->cStringUsingEncoding(4);
                info[name] = text;
            }
        }
        count = (unsigned)reinterpret_cast<uintptr_t>(objc_msgSend(
            userInfo, "countByEnumeratingWithState:objects:count:", &state, objects, 0x10));
    }
    getCurrentMIDlet()->receivedNotification(info);
}

void RuntimeIPhone::applicationWillTerminate(void*) {
    CleanIPSP(reinterpret_cast<void*>(destroyMIDlet(true) ? 1 : 0));
}

void RuntimeIPhone::applicationDidReceiveMemoryWarning(void*) {
    DisplayIPhone::getStaticDisplay()->memoryWarning();
}

void RuntimeIPhone::applicationDidFinishLaunching(void*) {
    union {
        unsigned bits;
        float value;
    } factor;
    factor.bits = 0x3df5c28fu;
    AccelerometerIPhone::getStaticAccelerometer()->setFilterFactor(factor.value);
    Display* display = DisplayIPhone::getStaticDisplay();
    setMIDlet(implementation_getMIDlet(), display);
    display->showNotify();
    display->serviceRepaints();
    util::Timer* timer = new util::Timer();
    RuntimeTimerTask* task = new RuntimeTimerTask();
    timer->schedule(task, 100, 100);
}

void RuntimeIPhone::scheduleTimerAtFixedRate(util::Timer* timer, long delay, long period) {
    addTimer(timer);
    CocoaTimer* cocoa = CocoaTimer::alloc()->init();
    cocoa->schedule_withDelay_andPeriod_(timer, delay, period);
}

void RuntimeIPhone::scheduleTimer(util::Timer* timer, long delay, long period) {
    addTimer(timer);
    CocoaTimer* cocoa = CocoaTimer::alloc()->init();
    cocoa->schedule_withDelay_andPeriod_(timer, delay, period);
}

void RuntimeIPhone::scheduleTimer(util::Timer* timer, long delay) {
    addTimer(timer);
    CocoaTimer* cocoa = CocoaTimer::alloc()->init();
    cocoa->scheduleOnce_withDelay_(timer, delay);
}

bool RuntimeIPhone::eventLoop() {
    return Runtime::eventLoop();
}

String* RuntimeIPhone::getOSVersion() {
    const __CFString* version =
        reinterpret_cast<const __CFString*>(UIDevice::currentDevice()->systemVersion());
    CFRetain(version);
    return new String(version);
}

bool RuntimeIPhone::isOtherAudioPlaying() {
    if (m_otherAudio == 0) {
        MPMusicPlayerController* player = MPMusicPlayerController::iPodMusicPlayer();
        if (!player) {
            void* context = alcGetCurrentContext();
            if (context) {
                puts("Clearing AL context for audio check");
                alcMakeContextCurrent(0);
                m_otherAudio = otherAudioState();
                alcMakeContextCurrent(context);
            } else {
                m_otherAudio = otherAudioState();
            }
        } else {
            m_otherAudio = player->playbackState() == 1 ? 1 : 2;
        }
    }
    return m_otherAudio == 1;
}

void RuntimeIPhone::resumeAudio() {
    if (!m_audioSuspended)
        return;
    unsigned category = kCategorySolo;
    if (isOtherAudioPlaying())
        category = kCategoryAmbient;
    if (AudioSessionSetProperty(kAudioCategory, 4, &category) != 0)
        NSLog(CFSTR("ERROR SETTING AUDIO CATEGORY!\n"));
    unsigned rate[2] = {0, kSampleRateHi};
    if (AudioSessionSetProperty(kHardwareSampleRate, 8, rate) != 0)
        NSLog(CFSTR("ERROR SETTING AUDIO SAMPLE RATE!\n"));
    if (AudioSessionSetActive(1) != 0)
        NSLog(CFSTR("ERROR SETTING AUDIO SESSION ACTIVE!\n"));
    void* context = PlayerIPhoneOpenAL::getSharedContext();
    if (context) {
        alcMakeContextCurrent(context);
        alcProcessContext(context);
    }
    m_audioSuspended = false;
    m_otherAudio = 0;
}

void RuntimeIPhone::applicationDidBecomeActive(void*) {
    resumeAudio();
    startMIDlet();
    DisplayIPhone::getStaticDisplay()->showNotify();
    pointerClearAll();
}

void RuntimeIPhone::suspendAudio() {
    AudioLock lock;
    if (m_audioSuspended)
        return;
    AudioSessionSetActive(0);
#ifndef MEDECOMP_PORT
    void* context = PlayerIPhoneOpenAL::getSharedContext();
    if (context) {
        alcMakeContextCurrent(0);
        alcSuspendContext(context);
    }
#else
#endif
    m_audioSuspended = true;
    m_otherAudio = 0;
    puts("RuntimeIPhone::suspendAudio done");
}

void RuntimeIPhone::applicationWillResignActive(void*) {
    pauseMIDlet();
#ifdef MEDECOMP_PORT
    if (!medecomp_background_audio())
#endif
    suspendAudio();
    DisplayIPhone::getStaticDisplay()->hideNotify();
}

bool RuntimeIPhone::isOSMediaPickerSupported() {
    return NSClassFromString(reinterpret_cast<NSString*>(
               const_cast<__CFString*>(CFSTR("MPMediaPickerController")))) != 0;
}

int RuntimeIPhone::totalMemory() {
    int mib[2] = {6, 5};
    int bytes;
    unsigned size = 4;
    if (sysctl(mib, 2, &bytes, &size, 0, 0) < 0)
        perror("getting physical memory");
    return bytes;
}

int RuntimeIPhone::freeMemory() {
    int mib[2] = {6, 6};
    int bytes;
    unsigned size = 4;
    if (sysctl(mib, 2, &bytes, &size, 0, 0) < 0)
        perror("getting user memory");
    return bytes;
}

RuntimeIPhone::~RuntimeIPhone() {
    setDeviceToken(0);
}

RuntimeIPhone::RuntimeIPhone()
    : m_unk78(0),
      m_deviceToken(0),
      m_pushStartup(false),
      m_platformType(0),
      m_audioSuspended(true),
      m_otherAudio(0),
      m_pushOptions(0) {
    m_platformType = UIDevice::currentDevice()->platformType();
}

}
