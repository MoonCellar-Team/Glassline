#include "runtime/midp/Runtime.h"

#include <stddef.h>

#include "runtime/midp/Display.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/MIDlet.h"
#include "runtime/midp/RuntimeClass.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "runtime/midp/System.h"
#include "runtime/util/Timer.h"

#include <new>

namespace midp {

Runtime* Runtime::getRuntime() {
    static RuntimeIPhone* runtime = NULL;
    if (runtime == NULL)
        runtime = new RuntimeIPhone();
    return runtime;
}

Class* Runtime::getClass() const {
    return new RuntimeClass();
}

Runtime::PointerState* Runtime::getPointerStatus(int pointerId) {
    switch (pointerId) {
    case 1:
        return &m_pointers[1];
    case 2:
        return &m_pointers[2];
    case 3:
        return &m_pointers[3];
    default:
        return &m_pointers[0];
    }
}

void Runtime::pointerClearAll() {
    for (int i = 0; i != 3; i++)
        getPointerStatus(i)->m_down = 0;
}

void Runtime::destroyMIDlet(MIDlet* midlet) {
    if (midlet == NULL)
        return;
    removeMIDlet(midlet);
}

bool Runtime::eventLoop() {
    return m_midlets.size() != 0;
}

MIDlet* Runtime::getCurrentMIDlet() const {
    if (m_midlets.size() <= 0)
        return NULL;
    return static_cast<MIDlet*>(m_midlets.lastElement());
}

bool Runtime::destroyMIDlet(bool unconditional) {
    MIDlet* midlet = getCurrentMIDlet();
    if (midlet == NULL)
        return false;
    INCREF(midlet);
    midlet->destroyApp(unconditional);
    removeMIDlet(midlet);
    return DECREF(midlet);
}

void Runtime::pauseMIDlet() {
    getCurrentMIDlet()->pauseApp();
}

void Runtime::startMIDlet() {
    getCurrentMIDlet()->startApp();
}

void Runtime::removeTimer(util::Timer* timer) {
    m_timers.removeElement(timer);
}

void Runtime::removeMIDlet(MIDlet* midlet) {
    m_midlets.removeElement(midlet);
}

void Runtime::addTimer(util::Timer* timer) {
    m_timers.addElement(timer);
}

void Runtime::addMIDlet(MIDlet* midlet) {
    m_midlets.addElement(midlet);
}

void Runtime::setMIDlet(MIDlet* midlet, Display* display) {
    MIDlet* current = getCurrentMIDlet();
    if (current != NULL)
        current->pauseApp();
    midlet->setDisplay(display);
    addMIDlet(midlet);
}

Displayable* Runtime::getCurrentDisplayable() const {
    MIDlet* midlet = getCurrentMIDlet();
    if (midlet == NULL)
        return NULL;
    Display* display = midlet->getDisplay();
    if (display == NULL)
        return NULL;
    return display->getCurrent();
}

void Runtime::pointerReleased(int x, int y, int pointerId) {
    PointerState* state = getPointerStatus(pointerId);
    state->m_down = 0;
    state->m_x = x;
    state->m_y = y;
    Displayable* current = getCurrentDisplayable();
    if (current != NULL)
        current->pointerReleased(x, y, pointerId);
}

void Runtime::pointerDragged(int x, int y, int pointerId) {
    PointerState* state = getPointerStatus(pointerId);
    state->m_x = x;
    state->m_y = y;
    Displayable* current = getCurrentDisplayable();
    if (current != NULL)
        current->pointerDragged(x, y, pointerId);
}

void Runtime::pointerPressed(int x, int y, int pointerId, int tapCount) {
    PointerState* state = getPointerStatus(pointerId);
    state->m_down = 1;
    state->m_x = x;
    state->m_y = y;
    state->m_tapCount = tapCount;
    Displayable* current = getCurrentDisplayable();
    if (current != NULL)
        current->pointerPressed(x, y, pointerId, tapCount);
}

Runtime::~Runtime() {
    System::println("~Runtime");
    while (m_midlets.size() > 0)
        destroyMIDlet(true);
    {
        util::VectorSE<Object> pending;
        if (m_timers.size() > 0) {
            for (int i = 0; i < m_timers.size(); i++)
                pending.addElement(m_timers.elementAt(i));
        }
        m_timers.removeAllElements();
        if (pending.size() > 0) {
            for (int i = 0; i < pending.size(); i++) {
                util::Timer* timer = static_cast<util::Timer*>(pending.elementAt(i));
                if (timer != NULL)
                    timer->cancel();
            }
        }
    }
    m_midlets.~VectorSE();
    m_timers.~VectorSE();
}

Runtime::Runtime() {
    new (&m_timers) util::VectorSE<Object>();
    new (&m_midlets) util::VectorSE<Object>();
    m_pointers[0].m_down = 0;
    m_pointers[1].m_down = 0;
    m_pointers[2].m_down = 0;
    m_pointers[3].m_down = 0;
}

}
