#include "game/UI/ChapterArrowedScroller.h"

static const int LIST_HEIGHT = 60;
static const int ARROW_GAP = 48;
static const int RENDER_EXTRA = 1;

namespace UI {

bool ChapterArrowedScroller::pointerPressed(int x, int y, int id) {
    if (contains(x, y)) {
        return NotchedSlider::pointerPressed(toRelativeX(x), y, id);
    }
    if (m_left.contains(x, y)) {
        return m_left.pointerPressed(x, y, id);
    }
    if (m_right.contains(x, y)) {
        return m_right.pointerPressed(x, y, id);
    }
    return false;
}

void ChapterArrowedScroller::set(bool looped, int x, int y, int notchWidth) {
    NotchedSlider::setLooped(looped);
    NotchedSlider::setNotchWidth(notchWidth);
    NotchedSlider::setRenderExtra(RENDER_EXTRA);
    setPosition(x, y);
    setDimensions(notchWidth, LIST_HEIGHT);

    int arrowY = getY() + ((LIST_HEIGHT - m_left.getHeight()) >> 1);
    m_left.setPosition(getX() - ARROW_GAP - m_left.getWidth(), arrowY);
    m_right.setPosition(notchWidth + getX() + ARROW_GAP, arrowY);
    m_right.setEnabled(canNext());
    m_left.setEnabled(canPrev());
}

bool ChapterArrowedScroller::pointerDragged(int x, int y, int id) {
    if (contains(x, y)) {
        NotchedSlider::pointerDragged(toRelativeX(x), y, id);
    } else if (isDragging()) {
        NotchedSlider::pointerReleased(x, y, id);
    }
    if (m_left.contains(x, y)) {
        return m_left.pointerPressed(x, y, id);
    }
    if (m_right.contains(x, y)) {
        return m_right.pointerPressed(x, y, id);
    }
    return false;
}

void ChapterArrowedScroller::update(int dt) {
    NotchedSlider::update(dt);
    m_left.setEnabled(canPrev());
    m_right.setEnabled(canNext());
}

ChapterArrowedScroller::ChapterArrowedScroller() {}

void ChapterArrowedScroller::render(midp::Graphics* g, int originY, int originX) {
    NotchedSlider::render(g, originY, originX);
    m_left.render(g, originY, originX);
    m_right.render(g, originY, originX);
}

bool ChapterArrowedScroller::pointerReleased(int x, int y, int id) {
    if (contains(x, y)) {
        return NotchedSlider::pointerReleased(x, y, id);
    }
    if (m_left.contains(x, y)) {
        prev();
        return m_left.pointerReleased(x, y, id);
    }
    if (m_right.contains(x, y)) {
        next();
        return m_right.pointerReleased(x, y, id);
    }
    return false;
}

WindowElement* ChapterArrowedScroller::getSelectedItem() const {
    return NotchedSlider::getSelectedItem();
}

void ChapterArrowedScroller::setLooped(bool looped) {
    NotchedSlider::setLooped(looped);
}

int ChapterArrowedScroller::getItemCount() const {
    return NotchedSlider::getItemCount();
}

ChapterArrowedScroller::~ChapterArrowedScroller() {}

void ChapterArrowedScroller::addItem(WindowElement* item) {
    NotchedSlider::addItem(item);
}

}
