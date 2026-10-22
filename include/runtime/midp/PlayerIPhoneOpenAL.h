#pragma once

#include "runtime/midp/PitchControl.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/PositionControl.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/VolumeControl.h"

struct OpaqueAudioFileID;

namespace midp {

class Control;
class SharedALBuffer;
class String;

class PlayerIPhoneOpenAL : public Player,
                           public PitchControl,
                           public PositionControl,
                           public VolumeControl {
public:
    static void createShared();
    static void* getSharedContext();
    static bool doesSharedContextExist();
    explicit PlayerIPhoneOpenAL(struct OpaqueAudioFileID* file);
    static void destroyShared();
    static void setListenerPosition(float x, float y, float z);
    static void setListenerVelocity(float x, float y, float z);
    static void setListenerOrientation(float atX, float atY, float atZ, float upX, float upY,
                                       float upZ);

    PlayerIPhoneOpenAL(SharedALBuffer* buffer);
    virtual ~PlayerIPhoneOpenAL();
    virtual Control* getControl(String* type) const;
    virtual bool isExclusive() const;
    virtual void update(bool exclusive);
    virtual Player* duplicate();
    virtual Class* getClass() const;
    virtual void close();
    virtual void deallocate();
    virtual long long getDuration() const;
    virtual long long getMediaTime() const;
    virtual void prefetch();
    virtual void realize();
    virtual void setLoopCount(int count);
    virtual long long setMediaTime(long long time);
    virtual void start();
    virtual void stop();
    void pause();
    void unpause();
    virtual VolumeControl* getVolumeControl() const;
    virtual PitchControl* getPitchControl() const;
    virtual PositionControl* getPositionControl() const;
    virtual void setPitchFactor(float factor);
    virtual float getPitchFactor() const;
    virtual void setPosition(float x, float y, float z);
    virtual void setVelocity(float x, float y, float z);
    void updateVelocity();
    void updatePosition();
    virtual void setReferenceDistance(float distance);
    virtual void setRolloffFactor(float factor);
    virtual void setSourceRelative(bool relative);
    virtual void setVolume(int volume);
    virtual int getVolume() const;
    void checkContext();

    static void* s_sharedContext;
    static void* s_sharedDevice;

    ReferenceCountedPointer<SharedALBuffer> m_buffer;
    unsigned int m_source;
    void* m_context;
    float m_x;
    float m_y;
    float m_z;
    float m_vx;
    float m_vy;
    float m_vz;
};

}
