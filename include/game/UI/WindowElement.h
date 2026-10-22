#pragma once

class QuadManager;
namespace midp {
class Graphics;
}

namespace midp {
class Graphics;
}

namespace UI {

class Window;

class WindowElement {
public:
    WindowElement();
    WindowElement(int x, int y, int width, int height);
    virtual ~WindowElement();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX) = 0;
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void setX(int x);
    virtual void setY(int y);
    virtual void setWidth(int width);
    virtual void setHeight(int height);
    virtual void setPosition(int x, int y);
    virtual void setDimensions(int width, int height);
    virtual int getX() const;
    virtual int getY() const;
    virtual int getWidth() const;
    virtual int getHeight() const;
    virtual bool contains(int x, int y);
    virtual int toRelativeX(int x) const;
    virtual int toRelativeY(int y) const;

    int m_x;
    int m_y;
    int m_width;
    int m_height;
    Window* m_parent;
    QuadManager* m_quadManager;
};

}
