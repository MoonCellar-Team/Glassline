#include "game/UI/CheckBox.h"

#include "runtime/midp/Graphics.h"

static const int BOX_SIZE = 31;
static const int COLOR_FRAME = 0;
static const int COLOR_MARK = 0xff0000;

namespace UI {

void CheckBox::update(int dt) {}

void CheckBox::render(midp::Graphics* g, int originY, int originX) {
    int x = m_x + originX;
    int y = m_y + originY;
    g->setColor(COLOR_FRAME);
    g->drawRect(x, y, m_width - 1, m_height - 1);
    if (m_checked) {
        g->setColor(COLOR_MARK);
        g->fillRect(x + 4, y + 4, m_width - 8, m_height - 8);
    }
}

bool CheckBox::pointerReleased(int x, int y, int id) {
    m_checked = !m_checked;
    return true;
}

CheckBox::CheckBox()
    : WindowElement(0, 0, BOX_SIZE, BOX_SIZE),
      m_checked(false) {}

void CheckBox::setChecked(bool checked) {
    m_checked = checked;
}

bool CheckBox::getChecked() const {
    return m_checked;
}

CheckBox::~CheckBox() {}

}
