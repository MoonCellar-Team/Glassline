#include "game/GameUI/WindowBackingTitled.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "sdk/CoreGraphics.h"

namespace GameUI {

static const int FONT_TITLE = 16;
static const int FONT_HEADER = 17;
static const int TEXT_X = 6;
static const int TITLE_GAP = 16;
static const int HEADER_GAP = 6;
static const int ANCHOR_LEFT_TOP = 9;
static const int ANCHOR_LEFT_BASELINE = 0x41;

int WindowBackingTitled::getTop() {
    return m_backing.getTop();
}

int WindowBackingTitled::getHeight() {
    return m_backing.getHeight();
}

WindowBackingTitled::WindowBackingTitled(int titleId, int headerId)
    : m_title(), m_header(), m_titleY(0), m_headerY(0), m_titleHeight(0), m_backing() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_title = text->getString(titleId)->toUpperCase();
    m_header = text->getString(headerId)->toUpperCase();
    int titleBaseline = text->getBaseline(FONT_TITLE);
    m_titleY = -TITLE_GAP - titleBaseline;
    m_headerY = -TITLE_GAP - HEADER_GAP - titleBaseline;
    m_titleHeight = text->getBaseline(FONT_HEADER) - m_headerY;
}

WindowBackingTitled::~WindowBackingTitled() {}

void WindowBackingTitled::render2D(midp::Graphics* g) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int top = m_backing.getTop();
    int x = m_backing.getLeft() + TEXT_X;
    text->drawString(g, m_header, FONT_HEADER, x, m_headerY + top, ANCHOR_LEFT_BASELINE);
    text->drawString(g, m_title, FONT_TITLE, x, m_titleY + top, ANCHOR_LEFT_TOP);
}

void WindowBackingTitled::render3D(m3g::Graphics3D* g3d) {
    m_backing.render3D(g3d);
}

void WindowBackingTitled::setBounds(int left, int top, int width, int height, int withTitle) {
    if (withTitle != 0) {
        height -= m_titleHeight;
        top += m_titleHeight;
    }
    m_backing.setBounds(left, top, width, height);
}

}

__attribute__((unused)) static CGRect CGRectMake(float, float, float, float) {
    CGRect rect;
    rect.origin.x = 0.0f;
    rect.origin.y = 0.0f;
    rect.size.width = 1024.0f;
    rect.size.height = 768.0f;
    return rect;
}
