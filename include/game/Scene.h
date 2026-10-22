#pragma once

#include "runtime/midp/Runnable.h"

class AppEngine;
namespace midp {
class Class;
class Graphics;
}

class Scene : public midp::Runnable {
public:
    explicit Scene(AppEngine* engine);
    virtual ~Scene();
    virtual midp::Class* getClass() const;
    virtual void start(int param) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void end() = 0;
    virtual void render(midp::Graphics* g) = 0;
    virtual void pointerDragged(int x, int y, int id) = 0;
    virtual void pointerPressed(int x, int y, int id, int) = 0;
    virtual void pointerReleased(int x, int y, int id) = 0;
    virtual void update(int dt) = 0;

    AppEngine* m_engine;
    int m_loadingProgress;
};
