#pragma once

#include "runtime/midp/Thread.h"

class AppEngine;

class AppEngineThread : public midp::Thread {
public:
    explicit AppEngineThread(AppEngine* engine);
    virtual ~AppEngineThread();
    virtual void run();

    long long m_lastTime;
    AppEngine* m_engine;
};
