#pragma once

#include <pthread.h>

#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/Runnable.h"

class SoundManager;
namespace midp {
class Class;
}

class BGMusic : public midp::PlayerListener, public midp::Runnable {
public:
    static const int PLAY_RESTART = 1;
    static const int PLAY_LOOP = 2;
    static const int MUSIC_GROUP = 0;

    enum State {
        STATE_NONE = 0,
        STATE_PLAYING = 1,
        STATE_STOPPING = 2,
        STATE_STOPPED = 3,
        STATE_UNLOADED = 4,
        STATE_LOAD = 5,
    };

    explicit BGMusic(SoundManager& soundManager);
    virtual ~BGMusic();
    virtual void playerUpdate(const midp::Player* player, const char* event, midp::Object* data);
    virtual midp::Class* getClass() const;
    virtual void run();

    bool isUpdated();
    void close();
    void playMusic(int id, int flags);
    void restartMusic();
    void suspend();
    void resume();
#ifdef MEDECOMP_PORT
    midp::Player* musicPlayer();
#endif
    void beQuiet();
    void refreshVolume();
    void update(int dt);

    SoundManager* m_soundManager;
    bool m_audible;
    bool m_enabled;
    bool m_suspended;
    bool m_waitOtherAudio;
    int m_otherAudioTimer;
    bool m_forceRestart;
    bool m_closed;
    bool m_updated;
    State m_state;
    int m_loadDelay;
    int m_currentId;
    int m_handle;
    int m_requestedId;
    bool m_loop;
    long m_lastTime;
    pthread_mutex_t m_mutex;
};
