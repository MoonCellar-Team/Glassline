#include "game/UI/Window.h"

#include <algorithm>
#include <math.h>
#include <stddef.h>

#include "game/QuadManager.h"
#include "game/UI/HorizontalScrollBar.h"
#include "game/UI/VerticalScrollBar.h"
#include "runtime/midp/Graphics.h"

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;
static const int CLIENT_MARGIN = 10;
static const int CONTENT_PADDING = 5;
static const int TEXT_NULL = 0x800;
static const int QUAD_GROUP_FRAME = 71;

namespace UI {

void Window::close(WindowResult result) {
    m_closed = true;
    m_result = result;
}

void Window::setDeleteOnClosed(bool value) {
    m_deleteOnClosed = value;
}

void Window::clearClosed() {
    m_closed = false;
    m_result = RESULT_NONE;
}

void Window::setClientOffsetX(int offset) {
    if (offset > 0) {
        offset = 0;
    }
    m_clientOffsetX = offset;
    if (m_clientMaxX < -offset) {
        m_clientOffsetX = -m_clientMaxX;
    }
}

void Window::setClientOffsetY(int offset) {
    if (offset > 0) {
        offset = 0;
    }
    m_clientOffsetY = offset;
    if (m_clientMaxY < -offset) {
        m_clientOffsetY = -m_clientMaxY;
    }
}

WindowElement* Window::getElement(int index) {
    if ((unsigned)index < m_elements.size()) {
        return m_elements[index];
    }
    return NULL;
}

bool Window::pointerDragged(int x, int y, int id) {
    std::vector<WindowElement*>::iterator it = m_elements.begin();
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (; it != end; ++it) {
        WindowElement* element = *it;
        int elementX = element->getX();
        int elementY = element->getY();
        int elementWidth = element->getWidth();
        int elementHeight = element->getHeight();
        int localX = x - elementX - m_clientOffsetX;
        if (localX < elementWidth) {
            int localY = y - elementY - m_clientOffsetY;
            if (localY < elementHeight) {
                if (element->pointerDragged(localX, localY, id)) {
                    return true;
                }
                break;
            }
        }
    }
    int lastX = m_pointerX;
    int lastY = m_pointerY;
    m_pointerX = x;
    m_pointerY = y;
    setClientOffsetX(x - lastX + m_clientOffsetX);
    setClientOffsetY(y - lastY + m_clientOffsetY);
    return false;
}

bool Window::pointerReleased(int x, int y, int id) {
    std::vector<WindowElement*>::iterator it = m_elements.begin();
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (; it != end; ++it) {
        WindowElement* element = *it;
        int elementX = element->getX();
        int elementY = element->getY();
        int elementWidth = element->getWidth();
        int elementHeight = element->getHeight();
        int localX = x - elementX - m_clientOffsetX;
        if (localX < elementWidth) {
            int localY = y - elementY - m_clientOffsetY;
            if (localY < elementHeight) {
                if (element->pointerReleased(localX, localY, id)) {
                    return true;
                }
                break;
            }
        }
    }
    if (m_horizontalScrollBar != NULL) {
        m_horizontalScrollBar->setScrolling(false);
    }
    if (m_verticalScrollBar != NULL) {
        m_verticalScrollBar->setScrolling(false);
    }
    return false;
}

bool Window::pointerPressed(int x, int y, int id) {
    std::vector<WindowElement*>::iterator it = m_elements.begin();
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (; it != end; ++it) {
        WindowElement* element = *it;
        int elementX = element->getX();
        int elementY = element->getY();
        int elementWidth = element->getWidth();
        int elementHeight = element->getHeight();
        int localX = x - elementX - m_clientOffsetX;
        if (localX < elementWidth) {
            int localY = y - elementY - m_clientOffsetY;
            if (localY < elementHeight) {
                if (element->pointerPressed(localX, localY, id)) {
                    return true;
                }
                break;
            }
        }
    }
    if (m_horizontalScrollBar != NULL) {
        m_horizontalScrollBar->setScrolling(true);
    }
    if (m_verticalScrollBar != NULL) {
        m_verticalScrollBar->setScrolling(true);
    }
    m_pointerX = x;
    m_pointerY = y;
    return false;
}

void Window::update(int dt) {
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (std::vector<WindowElement*>::iterator it = m_elements.begin(); it != end; ++it) {
        (*it)->update(dt);
    }
    if (m_verticalScrollBar != NULL) {
        m_verticalScrollBar->update(dt);
    }
    if (m_horizontalScrollBar != NULL) {
        m_horizontalScrollBar->update(dt);
    }
}

Window::~Window() {
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (std::vector<WindowElement*>::iterator it = m_elements.begin(); it != end; ++it) {
        delete *it;
    }
    delete m_verticalScrollBar;
    delete m_horizontalScrollBar;
}

void Window::render(midp::Graphics* g, int originY, int originX) {
    int clipX = g->getClipX();
    int clipY = g->getClipY();
    int clipRight = clipX + g->getClipWidth();
    int clipBottom = clipY + g->getClipHeight();
    int left = std::max(m_clientX, clipX);
    int top = std::max(m_clientY, clipY);
    int right = std::min(m_clientX + m_clientWidth, clipRight);
    int bottom = std::min(m_clientY + m_clientHeight, clipBottom);
    g->setClip(left, top, right - left, bottom - top);
    std::vector<WindowElement*>::iterator it = m_elements.begin();
    std::vector<WindowElement*>::iterator end = m_elements.end();
    for (; it != end; ++it) {
        (*it)->render(g, m_clientY + m_clientOffsetY, m_clientX + m_clientOffsetX);
    }
    g->setClip(clipX, clipY, clipRight - clipX, clipBottom - clipY);
    m_quadManager->setGroupVisible(QUAD_GROUP_FRAME, false);
    if (m_verticalScrollBar != NULL) {
        m_verticalScrollBar->render(g, m_clientY, m_clientX);
    }
    if (m_horizontalScrollBar != NULL) {
        m_horizontalScrollBar->render(g, m_clientY, m_clientX);
    }
}

void Window::setX(int x) {
    WindowElement::setX(x);
    adjustClientArea();
}

void Window::setY(int y) {
    WindowElement::setY(y);
    adjustClientArea();
}

void Window::setWidth(int width) {
    WindowElement::setWidth(width);
    adjustClientArea();
}

void Window::setHeight(int height) {
    WindowElement::setHeight(height);
    adjustClientArea();
}

void Window::setPosition(int x, int y) {
    WindowElement::setPosition(x, y);
    adjustClientArea();
}

void Window::setDimensions(int width, int height) {
    WindowElement::setDimensions(width, height);
    adjustClientArea();
}

Window::Window(int x, int y, int width, int height)
    : WindowElement(x, y, width, height),
      m_unk1C(TEXT_NULL),
      m_marginX(CLIENT_MARGIN),
      m_marginY(CLIENT_MARGIN),
      m_clientX(m_marginX + x),
      m_clientY(m_marginY + y),
      m_clientWidth(width - m_marginX * 2),
      m_clientHeight(height - m_marginY * 2),
      m_clientMaxX(0),
      m_clientMaxY(0),
      m_clientOffsetX(0),
      m_clientOffsetY(0),
      m_pointerX(0),
      m_pointerY(0),
      m_contentWidth(0),
      m_contentHeight(0),
      m_verticalScrollBar(NULL),
      m_horizontalScrollBar(NULL),
      m_closed(false),
      m_result(RESULT_NONE),
      m_deleteOnClosed(true) {
    m_clientWidth = std::max(0, m_clientWidth);
    m_clientHeight = std::max(0, m_clientHeight);
}

Window::Window()
    : WindowElement(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
      m_unk1C(TEXT_NULL),
      m_marginX(CLIENT_MARGIN),
      m_marginY(CLIENT_MARGIN),
      m_clientX(m_marginX),
      m_clientY(m_marginY),
      m_clientWidth(SCREEN_WIDTH - m_marginX * 2),
      m_clientHeight(SCREEN_HEIGHT - m_marginY * 2),
      m_clientMaxX(0),
      m_clientMaxY(0),
      m_clientOffsetX(0),
      m_clientOffsetY(0),
      m_pointerX(0),
      m_pointerY(0),
      m_contentWidth(0),
      m_contentHeight(0),
      m_verticalScrollBar(NULL),
      m_horizontalScrollBar(NULL),
      m_closed(false),
      m_result(RESULT_NONE),
      m_deleteOnClosed(true) {}

WindowElement* Window::removeElement(WindowElement* element) {
    std::vector<WindowElement*>::iterator it = std::find(m_elements.begin(), m_elements.end(), element);
    if (it == m_elements.end()) {
        return NULL;
    }
    m_elements.erase(it);
    return element;
}

void Window::showVerticalScrollbar() {
    if (m_verticalScrollBar == NULL) {
        m_verticalScrollBar = new VerticalScrollBar(this);
        m_verticalScrollBar->setX(m_clientWidth - m_verticalScrollBar->getWidth());
        m_verticalScrollBar->setY(0);
        m_verticalScrollBar->setHeight(m_clientHeight);
        m_clientWidth -= m_verticalScrollBar->getWidth();
        m_clientMaxX = (int)fmax(0.0, m_contentWidth - m_clientWidth);
        adjustScrollBars();
    }
}

void Window::adjustScrollBars() {
    if (m_clientMaxX != 0) {
        showHorizontalScrollbar();
    } else if (m_horizontalScrollBar != NULL) {
        delete m_horizontalScrollBar;
        m_horizontalScrollBar = NULL;
    }
    if (m_clientMaxY != 0) {
        showVerticalScrollbar();
    } else if (m_verticalScrollBar != NULL) {
        delete m_verticalScrollBar;
        m_verticalScrollBar = NULL;
    }
}

void Window::showHorizontalScrollbar() {
    if (m_horizontalScrollBar == NULL) {
        m_horizontalScrollBar = new HorizontalScrollBar(this);
        m_horizontalScrollBar->setY(m_clientHeight - m_horizontalScrollBar->getHeight());
        m_horizontalScrollBar->setX(0);
        m_horizontalScrollBar->setWidth(m_clientWidth);
        m_clientHeight -= m_horizontalScrollBar->getHeight();
        m_clientMaxY = (int)fmax(0.0, m_contentHeight - m_clientHeight);
        adjustScrollBars();
    }
}

void Window::adjustClientArea() {
    m_clientX = m_marginX + m_x;
    m_clientY = m_marginY + m_y;
    m_clientWidth = m_width - m_marginX * 2;
    m_clientHeight = m_height - m_marginY * 2;
    m_clientMaxX = (int)fmax(0.0, m_contentWidth - m_clientWidth);
    m_clientMaxY = (int)fmax(0.0, m_contentHeight - m_clientHeight);
    adjustScrollBars();
}

WindowElement* Window::addElement(WindowElement* element) {
    int y = element->getY();
    int x = element->getX();
    int height = element->getHeight();
    int width = element->getWidth();
    int right = x + width + CONTENT_PADDING;
    if (m_contentWidth < right) {
        m_contentWidth = right;
    }
    int bottom = y + height + CONTENT_PADDING;
    if (m_contentHeight < bottom) {
        m_contentHeight = bottom;
    }
    m_elements.push_back(element);
    adjustClientArea();
    return element;
}

void Window::render(midp::Graphics* g) {
    render(g, 0, 0);
}

int Window::getClientOffsetX() const {
    return m_clientOffsetX;
}

int Window::getClientOffsetY() const {
    return m_clientOffsetY;
}

int Window::getClientMaxX() const {
    return m_clientMaxX;
}

int Window::getClientMaxY() const {
    return m_clientMaxY;
}

int Window::getClientWidth() const {
    return m_clientWidth;
}

int Window::getClientHeight() const {
    return m_clientHeight;
}

bool Window::isClosed() const {
    return m_closed;
}

WindowResult Window::getWindowResult() const {
    return m_result;
}

bool Window::deleteOnClosed() const {
    return m_deleteOnClosed;
}

}
