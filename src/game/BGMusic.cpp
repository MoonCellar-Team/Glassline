#include "game/BGMusic.h"

#include <string.h>

#include "game/BGMusicClass.h"
#include "game/SoundManager.h"
#include "runtime/midp/Manager.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"
#include "runtime/midp/System.h"
#include "runtime/midp/Thread.h"

#ifdef MEDECOMP_PORT
#include <stdio.h>

#include "runtime/midp/Player.h"

extern "C" int medecomp_background_audio(void);
#endif

bool BGMusic::isUpdated() {
    return m_updated;
}

midp::Class* BGMusic::getClass() const {
    return new BGMusicClass();
}

void BGMusic::close() {
    m_closed = true;
}

#ifdef MEDECOMP_PORT
midp::Player* BGMusic::musicPlayer() {
    if (m_handle < 0) {
        return NULL;
    }
    int id = m_soundManager->getHandleEventId(m_handle);
    int instance = m_soundManager->getHandleInstanceId(m_handle);
    if (id < 0 || id >= m_soundManager->m_players.m_length || instance < 0 ||
        instance >= m_soundManager->m_players[id].m_length) {
        return NULL;
    }
    return m_soundManager->m_players[id][instance];
}
#endif

void BGMusic::playerUpdate(const midp::Player* player, const char* event, midp::Object* data) {
    midp::String name(event);
#ifdef MEDECOMP_PORT
    if (m_suspended && name.equals(midp::PlayerListener::STOPPED)) {
        return;
    }
#endif
    if (name.equals(midp::PlayerListener::STOPPED)) {
        m_state = STATE_STOPPED;
    } else if (name.equals(midp::PlayerListener::END_OF_MEDIA)) {
        if (m_state == STATE_PLAYING) {
            m_state = STATE_STOPPED;
        }
    } else if (name.equals(midp::PlayerListener::CLOSED)) {
        m_state = STATE_LOAD;
        m_loadDelay = 250;
    }
}

void BGMusic::resume() {
    pthread_mutex_lock(&m_mutex);
#ifdef MEDECOMP_PORT
    if (m_suspended && m_state == STATE_PLAYING && m_handle >= 0) {
        m_suspended = false;
        if (midp::Player* player = musicPlayer()) {
            fprintf(stderr, "medecomp: музыка — дальше с того же места\n");
            player->start();
        }
        pthread_mutex_unlock(&m_mutex);
        return;
    }
#endif
    if (m_suspended) {
        m_suspended = false;
        m_waitOtherAudio = true;
        m_otherAudioTimer = 0;
        m_currentId = -1;
    }
    pthread_mutex_unlock(&m_mutex);
}

void BGMusic::restartMusic() {
    if (m_state != STATE_PLAYING) {
        return;
    }
    m_state = STATE_STOPPING;
    m_soundManager->stopEvent(m_handle);
}

void BGMusic::playMusic(int id, int flags) {
    m_audible = true;
    int previousId = m_requestedId;
    m_requestedId = id;
    m_loop = (flags & PLAY_LOOP) != 0;
    m_forceRestart = (flags & PLAY_RESTART) != 0;
    if (m_state == STATE_PLAYING) {
        if (m_forceRestart || previousId != id) {
            restartMusic();
        }
    } else if (m_state == STATE_NONE) {
        m_state = STATE_LOAD;
    }
}

void BGMusic::suspend() {
#ifdef MEDECOMP_PORT
    if (medecomp_background_audio()) {
        return;
    }
    pthread_mutex_lock(&m_mutex);
    if (!m_suspended) {
        m_suspended = true;
        midp::Player* player = m_state == STATE_PLAYING ? musicPlayer() : NULL;
        if (player != NULL) {
            player->stop();
            fprintf(stderr, "medecomp: музыка — пауза (без перемотки)\n");
        }
    }
    pthread_mutex_unlock(&m_mutex);
    return;
#endif
    pthread_mutex_lock(&m_mutex);
    if (!m_suspended) {
        m_suspended = true;
        if (m_state == STATE_PLAYING) {
            m_state = STATE_STOPPING;
            m_soundManager->stopEvent(m_handle);
            midp::Thread::sleep(200);
            m_state = STATE_UNLOADED;
            m_soundManager->unloadEvent(m_currentId);
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

void BGMusic::refreshVolume() {
    if (m_enabled && m_audible) {
        m_soundManager->setVolumeGroup(MUSIC_GROUP, SoundManager::MAX_VOLUME);
    } else {
        m_soundManager->setVolumeGroup(MUSIC_GROUP, SoundManager::MIN_VOLUME);
    }
}

void BGMusic::update(int dt) {
    if (m_suspended) {
        return;
    }
    pthread_mutex_lock(&m_mutex);
    if (m_waitOtherAudio) {
        m_otherAudioTimer -= dt;
        if (m_otherAudioTimer <= 0) {
            if (midp::Manager::isOtherAudioPlaying()) {
                m_otherAudioTimer = 250;
                if (!m_loop) {
                    m_audible = false;
                }
            } else {
                m_waitOtherAudio = false;
            }
        }
    }

    if (m_state == STATE_LOAD) {
        if (!m_waitOtherAudio && !m_suspended) {
            if (m_loadDelay > 0) {
                m_loadDelay -= dt;
            } else {
                m_audible = m_audible && (m_loop || m_currentId != m_requestedId || m_forceRestart);
                m_forceRestart = false;
                refreshVolume();
                m_currentId = m_requestedId;
                m_soundManager->loadEvent(m_currentId);
                m_soundManager->addPlayerListener(m_currentId, this);
                m_handle = m_soundManager->playEvent(m_currentId, 1.0f);
                m_state = STATE_PLAYING;
            }
        }
    } else if (m_state == STATE_STOPPED) {
        if (!m_waitOtherAudio && !m_suspended) {
            m_state = STATE_UNLOADED;
            m_soundManager->unloadEvent(m_currentId);
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

void BGMusic::run() {
    while (!m_closed) {
        long now = midp::System::currentTimeMillis();
        int dt = midp::Math::max(1L, now - m_lastTime);
        m_lastTime = now;
        update(dt);
        m_updated = true;
        midp::Thread::sleep(100);
    }
}

void BGMusic::beQuiet() {
    m_audible = false;
    refreshVolume();
}

BGMusic::~BGMusic() {
    pthread_mutex_destroy(&m_mutex);
}

BGMusic::BGMusic(SoundManager& soundManager)
    : m_soundManager(&soundManager),
      m_audible(false),
      m_enabled(true),
      m_suspended(false),
      m_waitOtherAudio(true),
      m_otherAudioTimer(0),
      m_forceRestart(false),
      m_closed(false),
      m_updated(false),
      m_state(STATE_NONE),
      m_loadDelay(0),
      m_currentId(-1),
      m_handle(-1),
      m_requestedId(-1),
      m_loop(false),
      m_lastTime(midp::System::currentTimeMillis()) {
    memset(&m_mutex, 0, sizeof(m_mutex));
    pthread_mutex_init(&m_mutex, NULL);
}
