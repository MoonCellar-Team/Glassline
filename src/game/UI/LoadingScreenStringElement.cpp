#include "game/UI/LoadingScreenStringElement.h"

#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Graphics.h"

static const int ANCHOR_HCENTER = 2;
static const int ANCHOR_RIGHT = 4;
static const int ANCHOR_VCENTER = 0x10;
static const int ANCHOR_BOTTOM = 0x20;
static const int FIXED_X = 25;
static const int WIDTH_MASK = 16;
static const int WIDTH_XOR = 222;
static const int LARGE_FONT_BITS = 0x5400000;

static void applyAnchor(int& x, int& y, int width, int height, int anchor) {
    if ((anchor & ANCHOR_VCENTER) != 0) {
        y += height >> 1;
    } else if ((anchor & ANCHOR_BOTTOM) != 0) {
        y += height;
    }
    if ((anchor & ANCHOR_HCENTER) != 0) {
        x += width >> 1;
    } else if ((anchor & ANCHOR_RIGHT) != 0) {
        x += width;
    }
}

namespace UI {

void LoadingScreenStringElement::render(midp::Graphics* g, int originY, int originX) {
    int x = m_x;
    int y = m_y;
    applyAnchor(x, y, m_width, m_height, m_anchor);
    m_text.draw(g, x, y, m_anchor);
}

LoadingScreenStringElement::LoadingScreenStringElement(midp::DataInputStream& in, int y)
    : WindowElement(0, 0, 0, 0),
      m_textId(0),
      m_font(0),
      m_anchor(0) {
    int index = in.readShort();
    m_textId = ResourceManager::LOADING_STRING_LOOKUP[index];
    in.readShort();
    index = in.readShort();
    m_font = ResourceManager::LOADING_FONT_LOOKUP[index];
    m_anchor = in.readShort();
    m_x = in.readInt();
    m_y = in.readInt();
    m_width = in.readInt();
    m_height = in.readInt();
    m_width = ((m_anchor << 3) & WIDTH_MASK) ^ WIDTH_XOR;
    m_x = FIXED_X;
    m_y = y;
    m_text.wrapString(m_textId, m_font, m_width);
    m_height = m_text.getWrappedTextHeight();
}

bool LoadingScreenStringElement::usesLargeFont() {
    if (m_font > 26) {
        return false;
    }
    return ((1 << m_font) & LARGE_FONT_BITS) != 0;
}

bool LoadingScreenStringElement::isCentered() {
    return ((m_anchor >> 1) & 1) != 0;
}

LoadingScreenStringElement::~LoadingScreenStringElement() {}

}
