#include "runtime/midp/Displayable.h"

#include <stddef.h>

#include "runtime/midp/CanvasClass.h"
#include "runtime/midp/Display.h"

namespace midp {

Class* Canvas::getClass() const {
    return new CanvasClass();
}

int Canvas::getGameAction(int keyCode) const {
    if (keyCode > 34) {
        if ((unsigned)(keyCode - 35) > 22)
            return 0;
        switch (keyCode) {
        case 50: return 1;
        case 52: return 2;
        case 53: return 8;
        case 54: return 5;
        case 56: return 6;
        default: return 0;
        }
    }
    if ((unsigned)keyCode > 12)
        return 0;
    if ((0x1f66 & (1 << keyCode)) != 0)
        return keyCode;
    return 0;
}

int Canvas::getKeyCode(int gameAction) const {
    if (gameAction > 34) {
        unsigned index = (unsigned)(gameAction - 35);
        if (index > 22)
            return 0;
        if ((0x007fe081u & (1u << index)) != 0)
            return gameAction;
        return 0;
    }
    switch (gameAction) {
    case 1: return 50;
    case 2: return 52;
    case 5: return 54;
    case 6: return 56;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12: return 53;
    default: return 0;
    }
}

bool Canvas::isDoubleBuffered() const {
    return true;
}

void Canvas::setFullScreenMode(bool) {}

bool Canvas::hasPointerEvents() {
    return false;
}

bool Canvas::hasPointerMotionEvents() {
    return false;
}

bool Canvas::hasRepeatEvents() {
    return false;
}

void Canvas::hideNotify() {}

void Canvas::showNotify() {}

bool Canvas::isCanvas() {
    return true;
}

void Canvas::sizeChanged(int width, int height) {
    Displayable::sizeChanged(width, height);
}

void Canvas::serviceRepaints() {
    Display* display = getDisplay();
    if (display != NULL)
        display->serviceRepaints();
}

void Canvas::repaint(int x, int y, int width, int height) {
    Display* display = getDisplay();
    if (display != NULL)
        display->repaint(x, y, width, height);
}

void Canvas::repaint() {
    repaint(0, 0, getWidth(), getHeight());
}

Canvas::~Canvas() {}

Canvas::Canvas() {}

}
