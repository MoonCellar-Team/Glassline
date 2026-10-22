#include "game/UI/HelpWindow.h"

#include "game/UI/HelpText.h"

static const int TEXT_TITLE = 0x803;
static const int TEXT_SUBTITLE = 0x95c;
static const int PANEL_X = 0x6e;
static const int PANEL_HEIGHT = 0x1d6;
static const int PANEL_WIDTH_CUT = 0xdc;

namespace UI {

static bool forwardToPanel(Window* panel, int x, int y, int id, bool (Window::*method)(int, int, int)) {
    if (!panel->contains(x, y)) {
        return false;
    }
    (panel->*method)(panel->toRelativeX(x), panel->toRelativeY(y), id);
    return true;
}

bool HelpWindow::pointerDragged(int x, int y, int id) {
    TitledWindow::pointerDragged(x, y, id);
    return forwardToPanel(m_panel.get(), x, y, id, &Window::pointerDragged);
}

bool HelpWindow::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
    return forwardToPanel(m_panel.get(), x, y, id, &Window::pointerPressed);
}

void HelpWindow::update(int dt) {
    TitledWindow::update(dt);
    m_panel->update(dt);
}

void HelpWindow::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    m_panel->render(g, originY, originX);
}

bool HelpWindow::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    return forwardToPanel(m_panel.get(), x, y, id, &Window::pointerReleased);
}

HelpWindow::HelpWindow()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_panel(NULL) {
    Window* panel = new Window(PANEL_X, (m_height - PANEL_HEIGHT) >> 1, m_width - PANEL_WIDTH_CUT,
                               PANEL_HEIGHT);
    m_panel.reset(panel);
    m_background.setPosition(panel->getX(), panel->getY());
    m_background.setDimensions(panel->getWidth(), panel->getHeight());
    panel->addElement(new HelpText(m_panel));
}

HelpWindow::~HelpWindow() {}

}
