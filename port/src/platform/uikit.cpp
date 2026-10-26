#include <string>

#include "../audio/music.h"
#include "port/cf.h"
#include "port/objc.h"
#include "port/platform.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/ReferenceCounted.h"
#include "sdk/UIKit.h"

namespace port {
namespace cf {
std::string urlPath(const void* url);
}
}

using port::objc::Class;
using port::objc::Message;
using port::objc::NSObjectClass;
using port::objc::Object;

namespace {

Object* singleton(const Class* c) { return new Object(c); }

const Class kUIApplication = {{"UIApplication", nullptr, nullptr}, &NSObjectClass, {}};
const Class kUIDevice = {{"UIDevice", nullptr, nullptr}, &NSObjectClass, {}};
const Class kMPMusicPlayerController = {{"MPMusicPlayerController", nullptr, nullptr}, &NSObjectClass, {}};

struct VideoObject : Object {
    midp::PlayerIPhoneVideo* player = nullptr;
    const void* url = nullptr;
    VideoObject();
    ~VideoObject() {
        dropPlayer();
        if (url) CFRelease(url);
    }
    void dropPlayer() {
        if (!player) return;
        midp::PlayerIPhoneVideo* p = player;
        player = nullptr;
        midp::DECREF(p);
    }
};

void* videoInit(Object* self, Message& m) {
    VideoObject* v = static_cast<VideoObject*>(self);
    v->url = va_arg(*m.args, const void*);
    if (v->url) CFRetain(v->url);
    v->player = va_arg(*m.args, midp::PlayerIPhoneVideo*);
    if (v->player) midp::INCREF(v->player);
    return self;
}

void* videoClose(Object* self, Message&) {
    VideoObject* v = static_cast<VideoObject*>(self);
    if (v->player) {
        v->player->notifyListeners(midp::PlayerListener::END_OF_MEDIA, nullptr);
        v->player->notifyListeners(midp::PlayerListener::STOPPED, nullptr);
        v->dropPlayer();
    }
    return nullptr;
}

void* videoPlay(Object* self, Message&) {
    VideoObject* v = static_cast<VideoObject*>(self);
    fprintf(stderr, "medecomp: видео пока не показывается: %s\n", port::cf::urlPath(v->url).c_str());
    CFRetain(self);
    port::objc::onMainThread([self] {
        port::objc::send(self, "close");
        CFRelease(self);
    }, false);
    return nullptr;
}

void* nothing(Object*, Message&) { return nullptr; }

const Class kPlayerIPhone2 = {
    {"PlayerIPhone2", port::objc::destroyAs<VideoObject>, nullptr},
    &NSObjectClass,
    {
        {"initWithCppInterface::", videoInit},
        {"init", nothing},
        {"play", videoPlay},
        {"stop", nothing},
        {"close", videoClose},
    },
};

VideoObject::VideoObject() : Object(&kPlayerIPhone2) {}

void* videoAlloc(Object*, Message&) { return new VideoObject(); }

const Class kPlayerIPhone2Meta = {{"PlayerIPhone2 class", nullptr, nullptr}, &NSObjectClass, {{"alloc", videoAlloc}}};

}

extern "C" void* OBJC_CLASS_PlayerIPhone2 MEDECOMP_SYMBOL("OBJC_CLASS_$_PlayerIPhone2");
void* OBJC_CLASS_PlayerIPhone2 = singleton(&kPlayerIPhone2Meta);

UIApplication* UIApplication::sharedApplication() {
    static Object* app = singleton(&kUIApplication);
    return reinterpret_cast<UIApplication*>(app);
}

void UIApplication::setStatusBarOrientation(int) {}

void UIApplication::openURL(NSURL* url) {
    std::string u = port::cf::urlPath(url);
    if (u.empty()) return;
    if (u.find("://") == std::string::npos) u = "file://" + u;
    port::platform::openURL(u.c_str());
}

UIDevice* UIDevice::currentDevice() {
    static Object* device = singleton(&kUIDevice);
    return reinterpret_cast<UIDevice*>(device);
}

NSString* UIDevice::systemVersion() {
    static const void* version = port::cf::makeString(std::string("4.3"));
    return static_cast<NSString*>(const_cast<void*>(version));
}

NSString* UIDevice::platformType() { return nullptr; }

int UIDevice::orientation() const { return 3; }

class MPMusicPlayerController : public NSObject {
public:
    static MPMusicPlayerController* iPodMusicPlayer();
    int playbackState();
};

MPMusicPlayerController* MPMusicPlayerController::iPodMusicPlayer() {
    static Object* player = singleton(&kMPMusicPlayerController);
    return reinterpret_cast<MPMusicPlayerController*>(player);
}

int MPMusicPlayerController::playbackState() { return port::audio::ownMusicPlaying() ? 1 : 0; }
