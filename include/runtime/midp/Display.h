#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Class;
class Displayable;
class Graphics;
class MIDlet;

class Display : public Object {
public:
    Display(int width, int height);
    virtual ~Display();
    virtual Class* getClass() const;
    static Display* getDisplay(MIDlet* midlet);
    Displayable* getCurrent();
    virtual void showNotify();
    virtual void hideNotify();
    virtual void memoryWarning();
    virtual void serviceRepaints() = 0;
    virtual void refresh();
    virtual void repaint(int x, int y, int width, int height) = 0;
    virtual void setCurrent(Displayable* next);
    virtual bool vibrate(int duration);
    void setGraphics(Graphics* graphics);
    Graphics* getGraphics() const;
    int getWidth() const;
    int getHeight() const;
    void setOrientation(int orientation);
    int getOrientation() const;

    Displayable* m_current;
    Graphics* m_graphics;
    int m_width;
    int m_height;
    int m_orientation;
    bool m_shown;
    bool m_painting;
};

}
