#pragma once

#include <vector>

#include "game/UI/WindowElement.h"

namespace UI {

class HorizontalScrollBar;
class VerticalScrollBar;

enum WindowResult {
    RESULT_NONE = 0,
    RESULT_OK = 1,
    RESULT_CANCEL = 2,
};

class Window : public WindowElement {
public:
    Window();
    Window(int x, int y, int width, int height);
    virtual ~Window();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void setX(int x);
    virtual void setY(int y);
    virtual void setWidth(int width);
    virtual void setHeight(int height);
    virtual void setPosition(int x, int y);
    virtual void setDimensions(int width, int height);
    virtual void render(midp::Graphics* g);

    bool isClosed() const;
    WindowResult getWindowResult() const;
    bool deleteOnClosed() const;
    void setDeleteOnClosed(bool value);
    void close(WindowResult result);
    void clearClosed();

    WindowElement* addElement(WindowElement* element);
    WindowElement* removeElement(WindowElement* element);
    WindowElement* getElement(int index);

    void setClientOffsetX(int offset);
    void setClientOffsetY(int offset);
    int getClientOffsetX() const;
    int getClientOffsetY() const;
    int getClientMaxX() const;
    int getClientMaxY() const;
    int getClientWidth() const;
    int getClientHeight() const;

    void adjustClientArea();
    void adjustScrollBars();
    void showVerticalScrollbar();
    void showHorizontalScrollbar();

    int m_unk1C;
    int m_marginX;
    int m_marginY;
    int m_clientX;
    int m_clientY;
    int m_clientWidth;
    int m_clientHeight;
    int m_clientMaxX;
    int m_clientMaxY;
    int m_clientOffsetX;
    int m_clientOffsetY;
    int m_pointerX;
    int m_pointerY;
    std::vector<WindowElement*> m_elements;
    int m_contentWidth;
    int m_contentHeight;
    VerticalScrollBar* m_verticalScrollBar;
    HorizontalScrollBar* m_horizontalScrollBar;
    bool m_closed;
    WindowResult m_result;
    bool m_deleteOnClosed;
};

}
