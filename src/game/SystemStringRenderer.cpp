#include "game/SystemStringRenderer.h"

#include <stddef.h>

#include "runtime/midp/Font.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/StringBuffer.h"

static const int WHITE = 0xffffff;
static const int TEXT_PLAIN = 0;

void SystemStringRenderer::setColor(int color) {
    m_color = color;
}

int SystemStringRenderer::charsWidth(const unsigned short* chars, int offset, int length) const {
    return m_font->charsWidth(chars, offset, length);
}

int SystemStringRenderer::charWidth(unsigned short ch) const {
    return m_font->charWidth(ch);
}

int SystemStringRenderer::getBaselinePosition() const {
    return m_font->getBaselinePosition();
}

int SystemStringRenderer::getHeight() const {
    return m_font->getHeight();
}

int SystemStringRenderer::getLeading() const {
    return m_font->getLeading();
}

int SystemStringRenderer::stringWidth(midp::String* str) const {
    return m_font->stringWidth(str);
}

int SystemStringRenderer::substringWidth(midp::String* str, int offset, int length) {
    return m_font->substringWidth(str, offset, length);
}

void SystemStringRenderer::drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length, int x,
                                         int y, int anchor) {
    g->setFont(m_font);
    g->setColor(m_color);
    g->drawSubstring(str, offset, length, x, y, anchor, TEXT_PLAIN);
}

void SystemStringRenderer::drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor) {
    g->setFont(m_font);
    g->setColor(m_color);
    g->drawString(str, x, y, anchor, TEXT_PLAIN);
}

void SystemStringRenderer::drawString(midp::Graphics* g, midp::StringBuffer* str, int x, int y, int anchor) {
    g->setFont(m_font);
    g->setColor(m_color);
    g->drawString(str->toString(), x, y, anchor, TEXT_PLAIN);
}

SystemStringRenderer::~SystemStringRenderer() {
    midp::DECREF(m_font);
    m_font = NULL;
}

SystemStringRenderer::SystemStringRenderer(midp::Font* font) : m_font(font), m_color(WHITE) {
    midp::INCREF(font);
}

midp::Class* SystemStringRenderer::getClass() const {
    return NULL;
}
