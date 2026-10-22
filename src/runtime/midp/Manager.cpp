#include "runtime/midp/Manager.h"

#include <stddef.h>

#include "runtime/midp/Player.h"
#include "runtime/midp/PlayerIPhoneOpenAL.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/Runtime.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"
#include "runtime/util/VectorSE.h"
#include "sdk/AudioToolbox.h"
#include "sdk/CoreFoundation.h"
#include "sdk/Foundation.h"

namespace midp {

extern NSLock* s_Manager_lock;
extern util::VectorSE<Player> Manager_s_players;
extern util::VectorSE<Player> Manager_s_removeList;

namespace {

ReferenceCounted* refBase(Player* player) {
    return player;
}

const unsigned int kAudioFilePropertyFileFormat = 0x66666d74;
const unsigned int kAudioFilePropertyDataFormat = 0x64666d74;
const unsigned int kAudioFileWAVEType = 0x57415645;
const unsigned int kAudioFormatLinearPCM = 0x6c70636d;

}

bool Manager::isOtherAudioPlaying() {
    return static_cast<RuntimeIPhone*>(Runtime::getRuntime())->isOtherAudioPlaying();
}

void Manager::setListenerVelocity(float x, float y, float z) {
    PlayerIPhoneOpenAL::setListenerVelocity(x, y, z);
}

void Manager::setListenerOrientation(float atX, float atY, float atZ, float upX, float upY,
                                    float upZ) {
    PlayerIPhoneOpenAL::setListenerOrientation(atX, atY, atZ, upX, upY, upZ);
}

void Manager::setListenerPosition(float x, float y, float z) {
    PlayerIPhoneOpenAL::setListenerPosition(x, y, z);
}

Player* Manager::createDuplicatePlayer(Player* player) {
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    ReferenceCountedPointer<Player> held(player);
    Player* copy = NULL;
    if (held != NULL) {
        copy = held->duplicate();
        INCREF(refBase(copy));
        while (!s_Manager_lock->tryLock())
            Thread::yield();
        Manager_s_players.addElement(copy);
        s_Manager_lock->unlock();
    }
    pool->release();
    return copy;
}

Player* Manager::createPlayer(String* locator) {
    ReferenceCountedHardReference hold(locator);
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    Player* player = NULL;
    NSString* name = (NSString*)locator->getCFString();
    if (name->hasPrefix((NSString*)CFSTR("/")))
        name = name->substringFromIndex(1);
    NSString* path = NSBundle::mainBundle()->pathForResource_ofType_inDirectory(
        name, NULL, (NSString*)CFSTR("res"));
    if (path) {
        NSURL* url = NSURL::fileURLWithPath(path);
        if (path->hasSuffix((NSString*)CFSTR("m4v"))) {
            player = new PlayerIPhoneVideo((const __CFURL*)url);
        } else {
            OpaqueAudioFileID* file = NULL;
            if (AudioFileOpenURL((const ::__CFURL*)url, 1, 0, &file) == 0) {
                unsigned int size = 4;
                unsigned int fileFormat = 0;
                AudioFileGetProperty(file, kAudioFilePropertyFileFormat, &size, &fileFormat);
                bool lpcm = false;
                if (fileFormat == kAudioFileWAVEType) {
                    size = 0x28;
                    AudioStreamBasicDescription desc;
                    AudioFileGetProperty(file, kAudioFilePropertyDataFormat, &size, &desc);
                    lpcm = desc.mFormatID == kAudioFormatLinearPCM;
                }
                if (fileFormat == kAudioFileWAVEType && lpcm)
                    player = new PlayerIPhoneOpenAL(file);
                if (!player)
                    player = new PlayerIPhoneCoreAudio(file);
            }
        }
        if (player) {
            INCREF(refBase(player));
            while (!s_Manager_lock->tryLock())
                Thread::yield();
            Manager_s_players.addElement(player);
            s_Manager_lock->unlock();
        }
    }
    pool->release();
    return player;
}

void Manager::update() {
    while (!s_Manager_lock->tryLock())
        Thread::yield();
    if (Manager_s_players.size() > 0) {
        bool exclusive = false;
        for (int i = 0;; i++) {
            if (i >= Manager_s_players.size())
                break;
            Player* player = Manager_s_players.elementAt(i);
            if (player->getState() == Player::STARTED && player->isExclusive()) {
                exclusive = true;
                break;
            }
        }
        for (int i = 0; i < Manager_s_players.size(); i++) {
            Player* player = Manager_s_players.elementAt(i);
            player->update(exclusive);
            if (refBase(player)->getRefCount() <= 1)
                Manager_s_removeList.addElement(player);
        }
        for (int i = 0; i < Manager_s_removeList.size(); i++)
            Manager_s_players.removeElement(Manager_s_removeList.elementAt(i));
        Manager_s_removeList.removeAllElements();
        if (exclusive)
            PlayerIPhoneOpenAL::destroyShared();
    }
    s_Manager_lock->unlock();
}

}
