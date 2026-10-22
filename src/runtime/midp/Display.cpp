#include "runtime/midp/Display.h"

#include <stddef.h>

#include "runtime/midp/DisplayClass.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/MIDlet.h"
#include "runtime/midp/OpenGLESManager.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/Thread.h"

namespace midp {

Class* Display::getClass() const {
    return new DisplayClass();
}

Displayable* Display::getCurrent() {
    return m_current;
}

void Display::showNotify() {
    if (!m_shown && m_current != NULL)
        m_current->showNotify();
    m_shown = true;
}

void Display::hideNotify() {
    if (m_shown && m_current != NULL)
        m_current->hideNotify();
    m_shown = false;
}

void Display::memoryWarning() {
    if (m_current != NULL)
        m_current->memoryWarning();
}

int Display::getWidth() const {
    return m_width;
}

int Display::getHeight() const {
    return m_height;
}

void Display::setGraphics(Graphics* graphics) {
    INCREF(graphics);
    DECREF(m_graphics);
    m_graphics = graphics;
}

Graphics* Display::getGraphics() const {
    return m_graphics;
}

void Display::setOrientation(int orientation) {
    if (m_current != NULL && m_orientation != orientation)
        m_current->orientationChanged(orientation);
    m_orientation = orientation;
}

int Display::getOrientation() const {
    return m_orientation;
}

bool Display::vibrate(int) {
    return false;
}

void Display::refresh() {
    if (!m_shown)
        return;
    m_painting = true;
    Displayable* current = getCurrent();
    if (current != NULL && current->isCanvas()) {
        OpenGLESManager::flushDeletedTextures();
        Graphics* g = getGraphics();
        g->bind2D();
        static_cast<Canvas*>(current)->paint(g);
    }
    m_painting = false;
}

void Display::setCurrent(Displayable* next) {
    if (m_current == next)
        return;
    while (m_painting)
        Thread::yield();
    INCREF(next);
    if (m_current != NULL) {
        m_current->hideNotify();
        m_current->setDisplay(NULL);
        DECREF(m_current);
    }
    m_current = next;
    if (next == NULL)
        return;
    next->setDisplay(this);
    m_current->sizeChanged(getWidth(), getHeight());
    m_current->orientationChanged(getOrientation());
    if (m_shown)
        m_current->showNotify();
}

Display* Display::getDisplay(MIDlet* midlet) {
    return midlet->getDisplay();
}

Display::~Display() {
    DECREF(m_current);
    DECREF(m_graphics);
}

Display::Display(int width, int height)
    : m_current(NULL), m_graphics(NULL), m_width(width), m_height(height),
      m_orientation(0), m_shown(false), m_painting(false) {}

}
