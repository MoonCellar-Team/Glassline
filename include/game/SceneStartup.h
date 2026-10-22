#pragma once

#include "game/Scene.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Graphics;
class Thread;
}

class SceneStartup : public Scene {
public:
    enum LoadingState {
        LOADING_NONE = 0,
        LOADING_START = 1,
        LOADING_ASSETS = 2,
        LOADING_AUTO_START = 3,
        LOADING_DONE = 4,
    };

    enum ThreadState {
        THREAD_RUN = 0,
        THREAD_PAUSED = 1,
        THREAD_STOP = 2,
    };

    explicit SceneStartup(AppEngine* engine);
    virtual ~SceneStartup();
    virtual void run();
    virtual void start(int param);
    virtual void pause();
    virtual void resume();
    virtual void end();
    virtual void render(midp::Graphics* g);
    virtual void pointerDragged(int x, int y, int id);
    virtual void pointerPressed(int x, int y, int id, int);
    virtual void pointerReleased(int x, int y, int id);
    virtual void update(int dt);

    void checkForLevelAutoStart();
    void exitStartup();
    void updateLoadingState(int);
    void updateLoading(int dt);

    bool m_levelAutoStart;
    int m_loadingState;
    int m_time;
    int m_splashTime;
    midp::ReferenceCountedPointer<midp::Thread> m_loadingThread;
    int m_threadState;
};
