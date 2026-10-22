#pragma once

#include "runtime/midp/Object.h"
#include "runtime/util/VectorSE.h"

namespace util {
class Timer;
}

namespace midp {

class Class;
class Display;
class Displayable;
class MIDlet;

class Runtime : public Object {
public:
    static Runtime* getRuntime();
    MIDlet* getCurrentMIDlet() const;
    void destroyMIDlet(MIDlet* midlet);
    virtual void scheduleTimer(util::Timer* timer, long delay, long period) = 0;
    virtual Class* getClass() const;
    struct PointerState {
        int m_down;
        int m_x;
        int m_y;
        int m_tapCount;
    };
    PointerState* getPointerStatus(int pointerId);
    void pointerClearAll();
    bool destroyMIDlet(bool unconditional);
    virtual bool eventLoop();
    void pauseMIDlet();
    void startMIDlet();
    void removeTimer(util::Timer* timer);
    virtual void removeMIDlet(MIDlet* midlet);
    void addTimer(util::Timer* timer);
    void addMIDlet(MIDlet* midlet);
    void setMIDlet(MIDlet* midlet, Display* display);
    Displayable* getCurrentDisplayable() const;
    void pointerReleased(int x, int y, int pointerId);
    void pointerDragged(int x, int y, int pointerId);
    void pointerPressed(int x, int y, int pointerId, int tapCount);
    virtual ~Runtime();
    Runtime();

    union { util::VectorSE<Object> m_timers; char m_timersBytes[0x18]; };
    PointerState m_pointers[4];
    union { util::VectorSE<Object> m_midlets; char m_midletsBytes[0x18]; };
};

MIDlet* Graphics3D_getMIDlet();

}
