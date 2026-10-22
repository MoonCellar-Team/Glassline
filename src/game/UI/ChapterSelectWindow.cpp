#include "game/UI/ChapterSelectWindow.h"

static const int TEXT_TITLE = 0x819;
static const int TEXT_START = 0x82e;

static const int BACKGROUND_POS = 0x96;
static const int SLIDER_X = 0x96;
static const int SLIDER_Y = 0x86;
static const int SLIDER_WIDTH = 0x2d4;
static const int SLIDER_HEIGHT = 0xa0;
static const int ARROW_GAP = 0x30;
static const int RENDER_EXTRA = 1;
static const int START_GAP = 0x10;

namespace UI {

bool ChapterSelectWindow::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
    if (m_chapters.contains(x, y)) {
        m_chapters.pointerPressed(m_chapters.toRelativeX(x), y, id);
    } else if (m_prevButton.contains(x, y)) {
        m_prevButton.pointerPressed(x, y, id);
    } else if (m_nextButton.contains(x, y)) {
        m_nextButton.pointerPressed(x, y, id);
    } else if (m_startButton.contains(x, y)) {
        m_startButton.pointerPressed(x, y, id);
    }
    return true;
}

bool ChapterSelectWindow::pointerDragged(int x, int y, int id) {
    TitledWindow::pointerDragged(x, y, id);
    if (m_chapters.contains(x, y)) {
        m_chapters.pointerDragged(m_chapters.toRelativeX(x), y, id);
    } else if (m_chapters.isDragging()) {
        m_chapters.pointerReleased(x, y, id);
    }
    if (m_prevButton.contains(x, y)) {
        m_prevButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_nextButton.contains(x, y)) {
        m_nextButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_startButton.contains(x, y) || m_startButton.isPressed()) {
        return true;
    }
    m_startButton.unpress();
    return true;
}

void ChapterSelectWindow::update(int dt) {
    TitledWindow::update(dt);
    m_chapters.update(dt);
    m_nextButton.setEnabled(m_chapters.canNext());
    m_prevButton.setEnabled(m_chapters.canPrev());
}

void ChapterSelectWindow::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    m_chapters.render(g, originY, originX);
    m_prevButton.render(g, originY, originX);
    m_nextButton.render(g, originY, originX);
    m_startButton.render(g, originY, originX);
}

bool ChapterSelectWindow::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    if (m_chapters.contains(x, y)) {
        m_chapters.pointerReleased(x, y, id);
    } else if (m_prevButton.contains(x, y)) {
        m_chapters.prev();
        m_prevButton.pointerReleased(x, y, id);
    }
    if (m_nextButton.contains(x, y)) {
        m_chapters.next();
        m_nextButton.pointerReleased(x, y, id);
    } else if (m_nextButton.isPressed()) {
        m_nextButton.unpress();
    }
    if (m_startButton.contains(x, y)) {
        m_startButton.pointerReleased(x, y, id);
        onSelected();
        close(RESULT_NONE);
    } else if (m_startButton.isPressed()) {
        m_startButton.unpress();
    }
    return true;
}

ChapterSelectWindow::ChapterSelectWindow(int subtitle)
    : TitledWindow(TEXT_TITLE, subtitle),
      m_startButton(TEXT_START) {
    setShowBackground(false);
    m_background.setPosition(BACKGROUND_POS, BACKGROUND_POS);
    m_background.setDimensions(0, 0);
    m_chapters.setLooped(false);
    m_chapters.setNotchWidth(SLIDER_WIDTH);
    m_chapters.setRenderExtra(RENDER_EXTRA);
    m_chapters.setPosition(SLIDER_X, SLIDER_Y);
    m_chapters.setDimensions(SLIDER_WIDTH, SLIDER_HEIGHT);

    int arrowY = m_chapters.getY() + ((SLIDER_HEIGHT - m_prevButton.getHeight()) >> 1);
    m_prevButton.setPosition((m_chapters.getX() - ARROW_GAP) - m_prevButton.getWidth(), arrowY);
    m_nextButton.setPosition(m_chapters.getX() + m_chapters.getWidth() + ARROW_GAP, arrowY);
    m_nextButton.setEnabled(m_chapters.canNext());
    m_prevButton.setEnabled(m_chapters.canPrev());
    m_startButton.setPosition((m_backButton.getX() - START_GAP) - m_startButton.getWidth(),
                              m_backButton.getY());
}

ChapterSelectWindow::~ChapterSelectWindow() {}

}
