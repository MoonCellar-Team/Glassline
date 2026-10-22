#pragma once

#include "runtime/midp/Controllable.h"
#include "runtime/midp/VolumeControl.h"
#include "runtime/util/VectorSE.h"
#include "sdk/AudioToolbox.h"

namespace midp {

class Class;
class PitchControl;
class PlayerListener;
class PositionControl;
class String;
class VolumeControl;

class Player : public Controllable {
public:
    int getState() const;
    static const int CLOSED = 0;
    static const int UNREALIZED = 100;
    static const int REALIZED = 200;
    static const int STARTED = 400;
    void addPlayerListener(PlayerListener* listener);
    virtual void close() = 0;
    virtual void prefetch();
    virtual void realize() = 0;
    virtual void setLoopCount(int count);
    virtual long long setMediaTime(long long time) = 0;
    virtual void start();
    virtual void stop();
    virtual VolumeControl* getVolumeControl() const;
    virtual PitchControl* getPitchControl() const;
    virtual PositionControl* getPositionControl() const;
    void pause();
    void unpause();
    void stateTransition(int state);
    void setContentType(String* type);
    int getLoopCount() const;
    void notifyListeners(const char* event, Object* data) const;
    virtual Class* getClass() const;
    Player();
    virtual ~Player();
    virtual bool isExclusive() const = 0;
    virtual void update(bool exclusive) = 0;
    virtual Player* duplicate() = 0;

    int m_state;
    int m_loopCount;
    String* m_contentType;
    util::VectorSE<PlayerListener> m_listeners;
};

struct __CFURL;
class Control;

class PlayerIPhoneVideo : public Player {
public:
    explicit PlayerIPhoneVideo(const __CFURL* url);
    virtual ~PlayerIPhoneVideo();
    virtual Control* getControl(String* type) const;
    virtual bool isExclusive() const;
    virtual void update(bool running);
    virtual Player* duplicate();
    virtual void close();
    virtual void deallocate();
    virtual long long getDuration() const;
    virtual long long getMediaTime() const;
    virtual void prefetch();
    virtual void realize();
    virtual long long setMediaTime(long long time);
    virtual void start();
    virtual void stop();
    virtual void notifyListeners(const char* event, Object* data) const;

    bool m_waiting;
    void* m_impl;
};

struct OpaqueAudioQueueTimeline;

class PlayerIPhoneCoreAudio : public Player, public VolumeControl {
public:
    explicit PlayerIPhoneCoreAudio(struct OpaqueAudioFileID* file);
    virtual ~PlayerIPhoneCoreAudio();
    void propertyCB(OpaqueAudioQueue* queue, unsigned long property);
    void playbackCB(OpaqueAudioQueue* queue, AudioQueueBuffer* buffer, bool running);
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
    virtual long long setMediaTime(long long time);
    virtual void start();
    virtual void stop();
    virtual VolumeControl* getVolumeControl() const;
    virtual void setVolume(int volume);
    virtual int getVolume() const;
    void startActual();

    OpaqueAudioFileID* m_file;
    AudioStreamBasicDescription m_format;
    OpaqueAudioQueue* m_queue;
    OpaqueAudioQueueTimeline* m_timeline;
    int m_bufferCount;
    AudioQueueBuffer** m_buffers;
    int m_packet;
    unsigned m_packetDescs;
    float m_scale;
    mutable long long m_duration;
    bool m_exclusiveHeld;
    int m_timeOffset;
    mutable int m_volume;
};

void AudioQueue_property(void* user, OpaqueAudioQueue* queue, unsigned long property);
void AudioQueue_playback(void* user, OpaqueAudioQueue* queue, AudioQueueBuffer* buffer);

unsigned int CalculatePacketForTime(AudioStreamBasicDescription& format, double seconds);
void CalculateBytesForTime(AudioStreamBasicDescription& format, unsigned long maxPacketSize,
                           double seconds, unsigned long* outBufferSize, unsigned long* outNumPackets);

}

void propListener(void* client, unsigned long stream, unsigned long property, const void* value);
