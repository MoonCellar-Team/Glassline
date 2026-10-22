#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Display;
class Class;
class Graphics;

template <class T>
class array;

class Displayable : public virtual Object {
public:
    Displayable();
    virtual ~Displayable();
    virtual Class* getClass() const;
    virtual void showNotify();
    virtual void hideNotify();
    virtual void memoryWarning();
    virtual void sizeChanged(int width, int height);
    virtual void orientationChanged(int orientation);
    virtual void keyPressed(int keyCode);
    virtual void keyReleased(int keyCode);
    virtual void keyRepeated(int keyCode);
    virtual void pointerDragged(int x, int y, int pointerId);
    virtual void pointerPressed(int x, int y, int pointerId, int tapCount);
    virtual void pointerReleased(int x, int y, int pointerId);
    virtual bool getPointerPos(array<int>& pos, int pointerId) const;
    virtual int getPointerTapCount(int pointerId) const;
    virtual bool isCanvas();

    void setDisplay(Display* display);
    Display* getDisplay() const;
    int getWidth() const;
    int getHeight() const;
    int getOrientation() const;
    void transformRect(int* rect) const;

    Display* m_display;
    int m_width;
    int m_height;
    int m_orientation;
};

class Canvas : public Displayable {
public:
    virtual void repaint();
    virtual void serviceRepaints();
    virtual void setFullScreenMode(bool fullScreen);
    virtual void paint(Graphics* g) = 0;
    virtual int getGameAction(int keyCode) const;
    virtual int getKeyCode(int gameAction) const;
    virtual bool isDoubleBuffered() const;
    virtual bool hasPointerEvents();
    virtual bool hasPointerMotionEvents();
    virtual bool hasRepeatEvents();
    virtual void hideNotify();
    virtual void showNotify();
    virtual bool isCanvas();
    virtual void sizeChanged(int width, int height);
    virtual void repaint(int x, int y, int width, int height);
    Canvas();
    virtual ~Canvas();
    virtual Class* getClass() const;
};

}
