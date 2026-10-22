#pragma once

#include "runtime/midp/array.h"

namespace midp {
class Player;
class PlayerListener;
}

class SoundManager {
public:
    static const float MAX_VOLUME;
    static const float MIN_VOLUME;

    static const int GROUP_MUSIC = 1;
    static const int GROUP_SFX = 2;
    static const int EVENT_2D = 1;

    SoundManager();
    virtual ~SoundManager();

    void loadData();
    void pause();
    void resume();

    void loadEvent(int id);
    void unloadEvent(int id);
    bool isEventLoaded(int id);
    void addPlayerListener(int id, midp::PlayerListener* listener);

    int playEvent(int id, float volume);
    int playEventAt(int id, float x, float y, float z, float volume);
    void stopEvent(int handle);
    bool isHandlePlaying(int handle);
    void setLoopCountEvent(int handle, int count);
    void setVolumeEvent(int handle, float volume);
    void setEventPosition(int handle, float x, float y, float z);

    float getVolumeMusic() const;
    float getVolumeSFX() const;
    void setVolumeMusic(float volume);
    void setVolumeSFX(float volume);
    void setVolumeGlobal(float volume);
    void setVolumeGroup(int group, float volume);
    void setGroupPitch(int group, float pitch);

    void setListenerPosition(float x, float y, float z);
    void setListenerVelocity(float x, float y, float z);
    void setListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ);

    int getHandle(int id, int instance);
    int getHandleEventId(int handle);
    int getHandleInstanceId(int handle);

    int calcEventVolume(int id, int instance);
    void applyVolume(int id, int instance);
    midp::Player* loadPlayer(int id);

    midp::array<unsigned char> m_groupFlags;
    midp::array<short> m_eventResources;
    midp::array<unsigned char> m_eventGroups;
    midp::array<unsigned char> m_eventInstances;
    midp::array<unsigned char> m_eventFlags;
    midp::array<float> m_eventRefDistances;
    float m_volumeGlobal;
    float m_volumeMusic;
    float m_volumeSFX;
    midp::array<midp::array<midp::Player*> > m_players;
    midp::array<midp::array<float> > m_instanceVolumes;
    midp::array<float> m_groupVolumes;
};
