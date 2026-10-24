#include "runtime/midp/PlayerIPhoneOpenAL.h"

#include "platform/AudioLock.h"
#include "runtime/midp/PlayerIPhoneOpenALClass.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/SharedALBuffer.h"
#include "runtime/midp/String.h"

void* operator new(size_t);

extern "C" {

enum {
    AL_PITCH = 0x1003,
    AL_POSITION = 0x1004,
    AL_VELOCITY = 0x1006,
    AL_LOOPING = 0x1007,
    AL_BUFFER = 0x1009,
    AL_GAIN = 0x100A,
    AL_ORIENTATION = 0x100F,
    AL_SOURCE_STATE = 0x1010,
    AL_PLAYING = 0x1012,
    AL_PAUSED = 0x1013,
    AL_STOPPED = 0x1014,
    AL_SOURCE_RELATIVE = 0x202,
    AL_REFERENCE_DISTANCE = 0x1020,
    AL_ROLLOFF_FACTOR = 0x1021
};

const int kAlcMixerRate = 0x2001;

const int kPrefetched = 300;
const int kPaused = 500;

void* alcOpenDevice(const char* name);
void* alcCreateContext(void* device, const int* attrs);
void alcMakeContextCurrent(void* context);
void alcDestroyContext(void* context);
int alcCloseDevice(void* device);
void* alGetProcAddress(const char* name);
void alListener3f(int param, float x, float y, float z);
void alListenerfv(int param, const float* values);
void alSourcei(unsigned int source, int param, int value);
void alSourcef(unsigned int source, int param, float value);
void alSourcefv(unsigned int source, int param, const float* values);
void alGetSourcef(unsigned int source, int param, float* value);
void alGetSourcei(unsigned int source, int param, int* value);
void alSourcePlay(unsigned int source);
void alSourcePause(unsigned int source);
void alSourceStop(unsigned int source);
void alSourceRewind(unsigned int source);
int alGetError();

void (*alcMacOSXMixerOutputRate)(double rate);
extern void (*alBufferDataStatic)(unsigned int buffer, int format, const void* data, int size, int freq);

#ifdef MEDECOMP_PORT
void medecomp_audio_silent(unsigned int source, int state);
#endif

}

namespace midp {

void sourcePoolInit();
unsigned int sourcePoolGet();
void sourcePoolPut(unsigned int id);

void* PlayerIPhoneOpenAL::s_sharedContext;
void* PlayerIPhoneOpenAL::s_sharedDevice;

bool PlayerIPhoneOpenAL::doesSharedContextExist() {
    return s_sharedContext != 0;
}

void* PlayerIPhoneOpenAL::getSharedContext() {
    return s_sharedContext;
}

VolumeControl* PlayerIPhoneOpenAL::getVolumeControl() const {
    return const_cast<VolumeControl*>(static_cast<const VolumeControl*>(this));
}

PitchControl* PlayerIPhoneOpenAL::getPitchControl() const {
    return const_cast<PitchControl*>(static_cast<const PitchControl*>(this));
}

PositionControl* PlayerIPhoneOpenAL::getPositionControl() const {
    return const_cast<PositionControl*>(static_cast<const PositionControl*>(this));
}

bool PlayerIPhoneOpenAL::isExclusive() const {
    return false;
}

long long PlayerIPhoneOpenAL::getDuration() const {
    return 0;
}

long long PlayerIPhoneOpenAL::getMediaTime() const {
    return 0;
}

long long PlayerIPhoneOpenAL::setMediaTime(long long) {
    return 0;
}

void PlayerIPhoneOpenAL::unpause() {
    alSourcePlay(m_source);
    stateTransition(STARTED);
}

void PlayerIPhoneOpenAL::pause() {
    alSourcePause(m_source);
    stateTransition(kPaused);
}

void PlayerIPhoneOpenAL::setListenerVelocity(float x, float y, float) {
    AudioLock lock;
    if (getSharedContext() != 0)
        alListener3f(AL_VELOCITY, x, y, x);
}

void PlayerIPhoneOpenAL::setListenerPosition(float x, float y, float z) {
    AudioLock lock;
    if (getSharedContext() != 0)
        alListener3f(AL_POSITION, x, y, z);
}

void PlayerIPhoneOpenAL::close() {
    AudioLock lock;
    if (getState() == CLOSED)
        return;
    deallocate();
    m_buffer = ReferenceCountedPointer<SharedALBuffer>(static_cast<SharedALBuffer*>(0));
    stateTransition(CLOSED);
}

void PlayerIPhoneOpenAL::setListenerOrientation(float atX, float atY, float atZ, float upX,
                                               float upY, float upZ) {
    AudioLock lock;
    if (getSharedContext() == 0)
        return;
    const float orientation[6] = {atX, atY, atZ, upX, upY, upZ};
    alListenerfv(AL_ORIENTATION, orientation);
}

void PlayerIPhoneOpenAL::setSourceRelative(bool relative) {
    if (getSharedContext() != 0 && m_source != 0)
        alSourcei(m_source, AL_SOURCE_RELATIVE, relative ? 1 : 0);
}

void PlayerIPhoneOpenAL::setLoopCount(int count) {
    if (m_source == 0)
        return;
    alSourcei(m_source, AL_LOOPING, count < 0 ? 1 : 0);
}

void PlayerIPhoneOpenAL::setRolloffFactor(float factor) {
    if (m_source == 0)
        return;
    alSourcef(m_source, AL_ROLLOFF_FACTOR, factor);
}

void PlayerIPhoneOpenAL::setReferenceDistance(float distance) {
    if (m_source == 0)
        return;
    alSourcef(m_source, AL_REFERENCE_DISTANCE, distance);
}

void PlayerIPhoneOpenAL::setVolume(int volume) {
    if (m_source == 0)
        return;
    alSourcef(m_source, AL_GAIN, static_cast<float>(volume) * 0.01f);
}

void PlayerIPhoneOpenAL::setPitchFactor(float factor) {
    if (m_source == 0)
        return;
    alSourcef(m_source, AL_PITCH, factor);
}

void PlayerIPhoneOpenAL::updateVelocity() {
    if (m_source == 0)
        return;
    const float velocity[3] = {m_vx, m_vy, m_vz};
    alSourcefv(m_source, AL_VELOCITY, velocity);
}

void PlayerIPhoneOpenAL::setVelocity(float x, float y, float z) {
    m_vx = x;
    m_vy = y;
    m_vz = z;
    updateVelocity();
}

void PlayerIPhoneOpenAL::updatePosition() {
    if (m_source == 0)
        return;
    const float position[3] = {m_x, m_y, m_z};
    alSourcefv(m_source, AL_POSITION, position);
}

void PlayerIPhoneOpenAL::setPosition(float x, float y, float z) {
    m_x = x;
    m_y = y;
    m_z = z;
    updatePosition();
}

int PlayerIPhoneOpenAL::getVolume() const {
    if (m_source == 0)
        return 0;
    float gain = 0.0f;
    alGetSourcef(m_source, AL_GAIN, &gain);
    return static_cast<int>(gain * 100.0f);
}

float PlayerIPhoneOpenAL::getPitchFactor() const {
    float pitch = 1.0f;
    if (m_source != 0)
        alGetSourcef(m_source, AL_PITCH, &pitch);
    return pitch;
}

void PlayerIPhoneOpenAL::stop() {
    AudioLock lock;
    const int state = getState();
    if (state == STARTED || state == kPaused) {
        alSourceStop(m_source);
        stateTransition(REALIZED);
    }
}

void PlayerIPhoneOpenAL::update(bool) {
#ifdef MEDECOMP_PORT
    if (getState() == STARTED) {
        AudioLock lock;
        int state = 0;
        if (m_source != 0)
            alGetSourcei(m_source, AL_SOURCE_STATE, &state);
        if (getState() == STARTED && state != AL_PLAYING && state != AL_PAUSED && state != AL_STOPPED) {
            medecomp_audio_silent(m_source, state);
            if (m_source != 0)
                alSourceStop(m_source);
            stateTransition(REALIZED);
            return;
        }
    }
#endif
    if (getState() != STARTED || m_source == 0)
        return;
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state != AL_STOPPED)
        return;
    stop();
    alSourceRewind(m_source);
}

void PlayerIPhoneOpenAL::deallocate() {
    AudioLock lock;
    const int state = getState();
    if (state == STARTED || state == kPaused) {
        stop();
        stateTransition(REALIZED);
    }
    if (m_source == 0)
        return;
    alSourcei(m_source, AL_BUFFER, 0);
    sourcePoolPut(m_source);
    m_source = 0;
}

Control* PlayerIPhoneOpenAL::getControl(String* type) const {
    INCREF(type);
    Control* control = 0;
    if (type->equals("PitchControl"))
        control = static_cast<PitchControl*>(const_cast<PlayerIPhoneOpenAL*>(this));
    else if (type->equals("VolumeControl"))
        control = static_cast<VolumeControl*>(const_cast<PlayerIPhoneOpenAL*>(this));
    else if (type->equals("PositionControl"))
        control = static_cast<PositionControl*>(const_cast<PlayerIPhoneOpenAL*>(this));
    DECREF(type);
    return control;
}

Class* PlayerIPhoneOpenAL::getClass() const {
    return new PlayerIPhoneOpenALClass();
}

void PlayerIPhoneOpenAL::destroyShared() {
    AudioLock lock;
    void* context = s_sharedContext;
    s_sharedContext = 0;
    void* device = s_sharedDevice;
    s_sharedDevice = 0;
    if (context != 0)
        alcDestroyContext(context);
    if (device != 0)
        alcCloseDevice(device);
}

void PlayerIPhoneOpenAL::createShared() {
    AudioLock lock;
    sourcePoolInit();
    if (s_sharedDevice == 0) {
        s_sharedDevice = alcOpenDevice(0);
        alcMacOSXMixerOutputRate =
            reinterpret_cast<void (*)(double)>(alGetProcAddress("alcMacOSXMixerOutputRate"));
        if (alcMacOSXMixerOutputRate != 0)
            alcMacOSXMixerOutputRate(22050.0);
        alBufferDataStatic = reinterpret_cast<void (*)(unsigned int, int, const void*, int, int)>(
            alGetProcAddress("alBufferDataStatic"));
    }
    if (s_sharedContext == 0) {
        const int attrs[3] = {kAlcMixerRate, 22050, 0};
        s_sharedContext = alcCreateContext(s_sharedDevice, attrs);
        alcMakeContextCurrent(s_sharedContext);
    }
}

PlayerIPhoneOpenAL::~PlayerIPhoneOpenAL() {
    close();
}

Player* PlayerIPhoneOpenAL::duplicate() {
    return new PlayerIPhoneOpenAL(m_buffer);
}

void PlayerIPhoneOpenAL::prefetch() {
    Player::prefetch();
    if (getState() != REALIZED)
        return;
    AudioLock lock;
    checkContext();
    if (m_source == 0) {
        m_source = sourcePoolGet();
        if (m_source == 0) {
            stateTransition(kPrefetched);
            return;
        }
    }
    alSourcei(m_source, AL_LOOPING, 0);
    alSourcef(m_source, AL_GAIN, 1.0f);
    alSourcef(m_source, AL_REFERENCE_DISTANCE, 1.0f);
    alSourcef(m_source, AL_ROLLOFF_FACTOR, 1.0f);
    alSourcei(m_source, AL_BUFFER, m_buffer->getSharedBuffer());
    updatePosition();
    updateVelocity();
    stateTransition(kPrefetched);
}

void PlayerIPhoneOpenAL::realize() {
    if (getState() != UNREALIZED)
        return;
    AudioLock lock;
    checkContext();
    SharedALBuffer* buffer = m_buffer.operator->();
    if (!buffer->isRealized())
        buffer->realize();
    setContentType(m_buffer->getContentType());
    stateTransition(REALIZED);
}

void PlayerIPhoneOpenAL::start() {
    AudioLock lock;
    checkContext();
    Player::start();
    const int state = getState();
    if ((state == STARTED || state == kPaused) && m_source != 0) {
        int sourceState;
        alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);
        if (sourceState == AL_PLAYING)
            return;
        alSourceRewind(m_source);
    }
    if (m_source != 0) {
        alSourcePlay(m_source);
        alGetError();
        updatePosition();
        updateVelocity();
    }
    stateTransition(STARTED);
}

PlayerIPhoneOpenAL::PlayerIPhoneOpenAL(SharedALBuffer* buffer)
    : m_buffer(buffer),
      m_source(0),
      m_context(0),
      m_x(0),
      m_y(0),
      m_z(0),
      m_vx(0),
      m_vy(0),
      m_vz(0) {
    AudioLock lock;
    m_context = getSharedContext();
}

void PlayerIPhoneOpenAL::checkContext() {
    AudioLock lock;
    if (getState() == CLOSED)
        return;
    createShared();
    if (m_context == getSharedContext())
        return;
    m_context = getSharedContext();
    const int state = getState();
    if (state < kPrefetched) {
        if (state != REALIZED)
            return;
        m_buffer->invalidate();
        m_source = 0;
        stateTransition(UNREALIZED);
        realize();
        return;
    }
    if (state != kPrefetched && state != STARTED && state != kPaused)
        return;
    m_buffer->invalidate();
    m_source = 0;
    stateTransition(UNREALIZED);
    realize();
    prefetch();
}

PlayerIPhoneOpenAL::PlayerIPhoneOpenAL(OpaqueAudioFileID* file)
    : m_buffer(),
      m_source(0),
      m_context(0),
      m_x(0),
      m_y(0),
      m_z(0),
      m_vx(0),
      m_vy(0),
      m_vz(0) {
    AudioLock lock;
    m_context = getSharedContext();
    m_buffer = new SharedALBuffer(file);
}

}
