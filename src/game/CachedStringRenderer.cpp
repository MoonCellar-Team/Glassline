#include "game/CachedStringRenderer.h"

#include <stddef.h>
#include <string.h>

#include "game/StringRecord.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int ANCHOR_HCENTER = 2;
static const int ANCHOR_RIGHT = 4;
static const int ANCHOR_TOP_LEFT = 9;
static const int ANCHOR_VCENTER = 16;
static const int ANCHOR_BOTTOM = 32;
static const int ANCHOR_BASELINE = 64;

static const int TRANS_NONE = 0;

CachedStringRenderer::CachedStringRenderer(StringRenderer* renderer, int size)
    : m_renderer(renderer), m_records(NULL), m_order(NULL), m_size(size) {
    midp::INCREF(m_renderer);
    m_records = new StringRecord[m_size];
    m_order = new short[m_size];
    for (int i = 0; i < m_size; i++) {
        m_order[i] = m_size - 1 - i;
    }
}

int CachedStringRenderer::charsWidth(const unsigned short* chars, int offset, int length) const {
    return m_renderer->charsWidth(chars, offset, length);
}

int CachedStringRenderer::charWidth(unsigned short ch) const {
    return m_renderer->charWidth(ch);
}

int CachedStringRenderer::getBaselinePosition() const {
    return m_renderer->getBaselinePosition();
}

int CachedStringRenderer::getHeight() const {
    return m_renderer->getHeight();
}

int CachedStringRenderer::getLeading() const {
    return m_renderer->getLeading();
}

int CachedStringRenderer::stringWidth(midp::String* str) const {
    return m_renderer->stringWidth(str);
}

int CachedStringRenderer::substringWidth(midp::String* str, int offset, int length) {
    return m_renderer->substringWidth(str, offset, length);
}

void CachedStringRenderer::bufferedDrawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor) {
    midp::ReferenceCountedHardReference hold(str);
    if (str->getCFString() == NULL) {
        return;
    }
    int width = m_renderer->stringWidth(str);
    int fullHeight = m_renderer->getHeight();
    int leading = m_renderer->getLeading();
    int left, right, top, bottom;
    m_renderer->getStringTexturePadding(left, right, top, bottom);
    int height = fullHeight - leading;
#ifdef MEDECOMP_PORT
    if (leading > 0)
        bottom += leading;
#endif
    if (width <= 0 || height <= 0) {
        return;
    }
    int imageWidth = left + width + right;
    int imageHeight = top + height + bottom;
    int drawX = x - left;
    int drawY = y - top;
    if (anchor & ANCHOR_HCENTER) {
        drawX -= width / 2;
    } else if (anchor & ANCHOR_RIGHT) {
        drawX -= width;
    }
    if (anchor & ANCHOR_VCENTER) {
        drawY -= height / 2;
    } else if (anchor & ANCHOR_BOTTOM) {
        drawY -= height;
    } else if (anchor & ANCHOR_BASELINE) {
        drawY -= m_renderer->getBaselinePosition();
    }

    int index = -1;
    for (int i = 0; i < m_size; i++) {
        if (m_records[i].equals(str)) {
            index = i;
            break;
        }
    }
    int pos;
    if (index == -1) {
        pos = m_size - 1;
        index = m_order[pos];
#ifdef MEDECOMP_PORT
        midp::ReferenceCountedPointer<midp::Image> image(
            midp::createScaledImage(imageWidth, imageHeight, medecomp_text_scale()));
#else
        midp::ReferenceCountedPointer<midp::Image> image(midp::Image::createImage(imageWidth, imageHeight));
#endif
        midp::ReferenceCountedPointer<midp::Graphics> imageGraphics(image->getGraphics());
        m_records[index].set(str, image);
        m_renderer->drawString(imageGraphics, str, left, top, ANCHOR_TOP_LEFT);
    } else {
        pos = -1;
        for (int i = 0; i < m_size; i++) {
            if (m_order[i] == index) {
                pos = i;
                break;
            }
        }
    }
    if (pos != 0) {
        memmove(m_order + 1, m_order, pos * sizeof(short));
        m_order[0] = index;
    }
    g->drawRegion(m_records[index].getImage(), 0, 0, imageWidth, imageHeight, TRANS_NONE, drawX, drawY,
                  ANCHOR_TOP_LEFT);
}

void CachedStringRenderer::drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor) {
    bufferedDrawString(g, str, x, y, anchor);
}

void CachedStringRenderer::drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length, int x,
                                         int y, int anchor) {
    bufferedDrawString(g, str->substring(offset, offset + length), x, y, anchor);
}

void CachedStringRenderer::drawString(midp::Graphics* g, midp::StringBuffer* str, int x, int y, int anchor) {
    drawString(g, str->toString(), x, y, anchor);
}

CachedStringRenderer::~CachedStringRenderer() {
    midp::DECREF(m_renderer);
    if (m_records != NULL) {
        delete[] m_records;
        m_records = NULL;
    }
}

midp::Class* CachedStringRenderer::getClass() const {
    return NULL;
}
