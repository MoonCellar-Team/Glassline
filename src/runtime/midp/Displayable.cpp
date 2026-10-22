#include "runtime/midp/Displayable.h"

#include <stddef.h>

#include "runtime/midp/Display.h"
#include "runtime/midp/DisplayableClass.h"

namespace midp {

Class* Displayable::getClass() const {
    return new DisplayableClass();
}

Displayable::Displayable() : m_display(NULL), m_width(0), m_height(0), m_orientation(0) {}

void Displayable::setDisplay(Display* display) {
    INCREF(display);
    DECREF(m_display);
    m_display = display;
}

Display* Displayable::getDisplay() const {
    return m_display;
}

void Displayable::showNotify() {}

void Displayable::hideNotify() {}

void Displayable::memoryWarning() {}

void Displayable::sizeChanged(int width, int height) {
    m_width = width;
    m_height = height;
}

void Displayable::orientationChanged(int orientation) {
    m_orientation = orientation;
}

void Displayable::keyPressed(int) {}

void Displayable::keyReleased(int) {}

void Displayable::keyRepeated(int) {}

void Displayable::pointerDragged(int, int, int) {}

void Displayable::pointerPressed(int, int, int, int) {}

void Displayable::pointerReleased(int, int, int) {}

bool Displayable::getPointerPos(array<int>&, int) const {
    return false;
}

int Displayable::getPointerTapCount(int) const {
    return 0;
}

int Displayable::getWidth() const {
    if ((unsigned)m_orientation - 2u < 2u)
        return m_height;
    return m_width;
}

int Displayable::getHeight() const {
    if ((unsigned)m_orientation - 2u < 2u)
        return m_width;
    return m_height;
}

int Displayable::getOrientation() const {
    return m_orientation;
}

void Displayable::transformRect(int* rect) const {
    int x = rect[0];
    int y = rect[1];
    int w = rect[2];
    int h = rect[3];
    int orientation = getOrientation();
    if ((unsigned)orientation - 2u < 2u) {
        rect[0] = m_width - (y + h);
        rect[1] = x;
        rect[2] = h;
        rect[3] = w;
        return;
    }
    if (orientation == 1) {
        rect[0] = m_width - (x + w);
        rect[1] = m_height - (y + h);
    }
}

bool Displayable::isCanvas() {
    return false;
}

Displayable::~Displayable() {
    DECREF(m_display);
}

}
