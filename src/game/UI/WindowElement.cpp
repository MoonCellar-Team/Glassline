#include "game/UI/WindowElement.h"

#include <stddef.h>

#include "game/AppEngine.h"

namespace UI {

void WindowElement::update(int dt) {}

bool WindowElement::pointerPressed(int x, int y, int id) {
    return false;
}

bool WindowElement::pointerReleased(int x, int y, int id) {
    return false;
}

bool WindowElement::pointerDragged(int x, int y, int id) {
    return false;
}

void WindowElement::setX(int x) {
    m_x = x;
}

void WindowElement::setY(int y) {
    m_y = y;
}

void WindowElement::setWidth(int width) {
    m_width = width;
}

void WindowElement::setHeight(int height) {
    m_height = height;
}

void WindowElement::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
}

void WindowElement::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
}

int WindowElement::getX() const {
    return m_x;
}

int WindowElement::getY() const {
    return m_y;
}

int WindowElement::getWidth() const {
    return m_width;
}

int WindowElement::getHeight() const {
    return m_height;
}

bool WindowElement::contains(int x, int y) {
    int dx = x - m_x;
    int dy = y - m_y;
    return dx > 0 && dx < m_width && dy > 0 && dy < m_height;
}

WindowElement::~WindowElement() {}

WindowElement::WindowElement(int x, int y, int width, int height)
    : m_x(x),
      m_y(y),
      m_width(width),
      m_height(height),
      m_parent(NULL),
      m_quadManager(&AppEngine::getCanvas()->getQuadManager()) {}

WindowElement::WindowElement()
    : m_x(0),
      m_y(0),
      m_width(0),
      m_height(0),
      m_parent(NULL),
      m_quadManager(&AppEngine::getCanvas()->getQuadManager()) {}

int WindowElement::toRelativeX(int x) const {
    return x - m_x;
}

int WindowElement::toRelativeY(int y) const {
    return y - m_y;
}

}
