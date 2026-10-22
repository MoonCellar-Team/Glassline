#include "game/SceneStartup.h"

#include <stddef.h>

#include "game/AchievementData.h"
#include "game/AnimPlayer.h"
#include "game/AnimationManager.h"
#include "game/AppEngine.h"
#include "game/BGMusic.h"
#include "game/GameCommon.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/QuadManager.h"
#include "game/ReflectionMapShader.h"
#include "game/Util.h"
#include "m3g/Appearance.h"
#include "m3g/Graphics3D.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"

static const int SPLASH_TIME = 3000;

static const int SCENE_MENU = 2;
static const int SCENE_GAME = 3;
static const int MENU_STATE_TITLE = 2;
static const int MENU_STATE_LAUNCHING_CHALLENGE = 35;
static const int MENU_STATE_NO_STORAGE = 37;

static const unsigned long long MIN_FREE_STORAGE = 5 * 1024 * 1024;

static const char* const LEVEL_AUTO_START_FILE = "LevelAutoStart";
#ifdef MEDECOMP_PORT
extern "C" int medecomp_player_level_base(void);
extern "C" void medecomp_player_level_apply(int base);
extern "C" int medecomp_player_level_test(void);
#endif
static const short LEVEL_AUTO_START_PENDING = 0;
static const short LEVEL_AUTO_START_USED = -1;

static const int IMAGE_LOADING_RUN = 231;
static const int APPEARANCE_REFLECTION = 7;
static const int QUAD_SET_MENU = 4;
static const int QUAD_SET_SPLASH = 1;
static const int QUAD_ANIM_FADE = 11;

static const int PAUSED_SLEEP = 1000;
static const int LOADING_FRAME_SLEEP = 40;

void SceneStartup::start(int) {
    m_loadingState = LOADING_START;
    m_loadingProgress = 0;
}

void SceneStartup::pause() {
    if (m_threadState != THREAD_STOP) {
        m_threadState = THREAD_PAUSED;
    }
}

void SceneStartup::resume() {
    if (m_threadState == THREAD_PAUSED) {
        m_threadState = THREAD_RUN;
    }
}

void SceneStartup::pointerPressed(int, int, int, int) {}

void SceneStartup::pointerDragged(int, int, int) {}

void SceneStartup::pointerReleased(int, int, int) {}

static int splashTime() {
#ifdef MEDECOMP_PORT
    if (medecomp_player_level_test()) {
        return 0;
    }
#endif
    return SPLASH_TIME;
}

void SceneStartup::checkForLevelAutoStart() {
    midp::ReferenceCountedPointer<midp::InputStream> in(
        midp::Class::getResourceAsStream(new midp::String(LEVEL_AUTO_START_FILE)));
    if (in != NULL) {
        midp::DataInputStream data(in);
        if (data.readShort() == LEVEL_AUTO_START_PENDING) {
            m_levelAutoStart = true;
            signed char mode = data.readByte();
            short level = data.readShort();
            AppEngine::getLevelData()->setCurrentLevelByIndex((LevelData::GameMode)mode, level);
        }
    }
#ifdef MEDECOMP_PORT
    int playerBase = medecomp_player_level_base();
    if (playerBase >= 0) {
        m_levelAutoStart = true;
        medecomp_player_level_apply(playerBase);
    }
#endif
    if (m_levelAutoStart) {
        midp::ReferenceCountedPointer<midp::OutputStream> out(
            midp::OutputStream::getResourceAsStream(new midp::String(LEVEL_AUTO_START_FILE)));
        midp::DataOutputStream data(out);
        data.writeShort(LEVEL_AUTO_START_USED);
    }
}

void SceneStartup::render(midp::Graphics* g) {
#ifdef MEDECOMP_PORT
    if (medecomp_player_level_test()) {
        if (m_splashTime == -1) {
            m_splashTime = 0;
        }
        g->setColor(255, 255, 255);
        g->fillRect(0, 0, m_engine->getWidth(), m_engine->getHeight());
        return;
    }
#endif
    if (m_splashTime < SPLASH_TIME) {
        m_engine->getQuadManager().render(g);
        if (m_splashTime == -1) {
            m_splashTime = 0;
        }
    } else {
        int width = m_engine->getWidth();
        int height = m_engine->getHeight();
        g->setColor(255, 255, 255);
        g->fillRect(0, 0, width, height);
        m_engine->getLoadingRunAnimPlayer()->drawAnim(g, (width >> 3) * 7, (height >> 3) * 7);
    }
}

void SceneStartup::exitStartup() {
    AppEngine* engine = m_engine;
    if (m_levelAutoStart) {
        engine->changeScene(SCENE_GAME);
        return;
    }
    if (engine->launchedForChallenge()) {
        engine->changeScene(SCENE_MENU, MENU_STATE_LAUNCHING_CHALLENGE);
        return;
    }
    if (Util::getFreeFSBytes() < MIN_FREE_STORAGE) {
        m_engine->setStorageFull(true);
        m_engine->changeScene(SCENE_MENU, MENU_STATE_NO_STORAGE);
    } else {
        m_engine->setStorageFull(false);
        m_engine->changeScene(SCENE_MENU, MENU_STATE_TITLE);
    }
}

void SceneStartup::end() {
    if (m_loadingThread != NULL) {
        midp::ReferenceCountedPointer<midp::Thread> thread(m_loadingThread);
        m_threadState = THREAD_STOP;
        m_loadingThread = NULL;
        thread->join();
    }
}

void SceneStartup::updateLoadingState(int) {
    switch (m_loadingState) {
    case LOADING_START:
        break;
    case LOADING_ASSETS: {
        m_engine->doStartupLoading();
        AppEngine* engine = m_engine;
        QuadManager& quads = engine->getQuadManager();
        AnimationManager::loadImage(engine->getResourceManager(), IMAGE_LOADING_RUN);
        m_engine->getLoadingRunAnimPlayer()->startAnim(0, AnimPlayer::START_LOOP);
        if (m3g::g_usingGLES20) {
            m3g::Appearance* appearance = AppEngine::getM3GAssets()->getAppearance(APPEARANCE_REFLECTION);
            appearance->setCustomShader(AppEngine::getCanvas()->getReflectionShader());
        }
        AppEngine::getLevelData()->loadData();
        AppEngine::getAchievementData()->loadData();
        m_engine->loadLoadingAssets();
        BGMusic* music = m_engine->getBGMusic();
        while (!music->isUpdated()) {
            midp::Thread::yield();
        }
        m_engine->loadSounds();
        while (m_splashTime == -1) {
            midp::Thread::yield();
        }
        quads.loadQuads(QUAD_SET_MENU);
        quads.setAnimFrame(QUAD_ANIM_FADE, 1);
        break;
    }
    case LOADING_AUTO_START:
        checkForLevelAutoStart();
        break;
    case LOADING_DONE:
        if (m_splashTime > splashTime()) {
            m_threadState = THREAD_STOP;
            m_loadingThread = NULL;
        }
        return;
    default:
        return;
    }
    m_loadingState++;
}

void SceneStartup::run() {
    for (;;) {
        while (m_threadState == THREAD_RUN) {
            updateLoadingState(0);
        }
        if (m_threadState == THREAD_STOP) {
            return;
        }
        midp::Thread::sleep(PAUSED_SLEEP);
    }
}

void SceneStartup::updateLoading(int dt) {
    m_engine->updateLoading(dt);
    m_engine->getLoadingRunAnimPlayer()->updateAnim(dt);
    if (m_splashTime >= 0) {
        m_splashTime += dt;
    }
    if (m_loadingThread == NULL && m_threadState == THREAD_RUN) {
        if (!m_engine->isFading()) {
            m_loadingThread = new midp::Thread(this);
            m_threadState = THREAD_RUN;
            m_loadingThread->start();
        }
        return;
    }
    midp::Thread::sleep(LOADING_FRAME_SLEEP);
}

void SceneStartup::update(int dt) {
    m_time += dt;
    if (m_loadingState == LOADING_DONE && m_threadState == THREAD_STOP) {
        if (m_time >= splashTime() && !m_engine->isFading()) {
            exitStartup();
        }
    } else {
        updateLoading(dt);
    }
}

SceneStartup::~SceneStartup() {
    m_engine->getQuadManager().freeQuads(QUAD_SET_SPLASH);
}

SceneStartup::SceneStartup(AppEngine* engine)
    : Scene(engine),
      m_levelAutoStart(false),
      m_loadingState(LOADING_NONE),
      m_time(0),
      m_splashTime(-1),
      m_loadingThread(NULL),
      m_threadState(THREAD_RUN) {}
