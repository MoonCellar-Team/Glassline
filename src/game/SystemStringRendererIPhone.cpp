#include "game/SystemStringRendererIPhone.h"

#include "runtime/midp/Font.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int WHITE = 0xffffff;
static const int DEFAULT_SHADOW_COLOR = 0x30000000;

static const int TEXT_PLAIN = 0;
static const int TEXT_SHADOW = 1;
static const int TEXT_STROKE = 2;

static void addShadowPadding(int offset, int blur, int& before, int& after) {
    if (offset > 0) {
        if (blur - offset > 0) {
            before += blur - offset;
        }
        after += blur + offset;
    } else {
        before += blur - offset;
        if (blur + offset > 0) {
            after += blur + offset;
        }
    }
}

void SystemStringRendererIPhone::setStrokeColor(int color) {
    m_strokeColor = color;
}

int SystemStringRendererIPhone::getHeight() const {
    return m_font->getHeight() + 2;
}

int SystemStringRendererIPhone::stringWidth(midp::String* str) const {
    midp::Font* font = m_font->canRenderString(str) ? m_font : m_fallbackFont;
    return font->stringWidth(str);
}

void SystemStringRendererIPhone::getStringTexturePadding(int& left, int& right, int& top, int& bottom) {
    m_font->getLayoutPadding(left, right, top, bottom);
    if (m_dropShadow) {
        addShadowPadding(m_shadowX, m_shadowBlur, left, right);
        addShadowPadding(m_shadowY, m_shadowBlur, bottom, top);
    }
}

void SystemStringRendererIPhone::setIPhoneRenderingStyle(int style) {
    if (style >= STYLE_FILL && style <= STYLE_FILL_STROKE) {
        m_renderingStyle = style;
    }
}

void SystemStringRendererIPhone::useIPhoneFontDropShadow(bool enable) {
    m_dropShadow = enable;
}

void SystemStringRendererIPhone::setIPhoneDropShadowParameters(int x, int y, int blur) {
    m_shadowX = x;
    m_shadowY = -y;
    m_shadowBlur = blur;
}

int SystemStringRendererIPhone::substringWidth(midp::String* str, int offset, int length) {
    midp::ReferenceCountedPointer<midp::String> sub(str->substring(offset, offset + length));
    midp::Font* font = m_font->canRenderString(sub) ? m_font : m_fallbackFont;
    return font->stringWidth(sub);
}

void SystemStringRendererIPhone::drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length,
                                               int x, int y, int anchor) {
    g->setFont(m_font->canRenderString(str) ? m_font : m_fallbackFont);
    g->setColor(m_color);
    int style = TEXT_PLAIN;
    if (m_dropShadow) {
        g->setFontDropShadowParameters(m_shadowX, m_shadowY, m_shadowBlur, m_shadowColor);
        style = TEXT_SHADOW;
    }
    if (m_renderingStyle == STYLE_FILL_STROKE) {
        g->drawSubstring(str, offset, length, x, y + 1, anchor, style);
        g->setColor(m_strokeColor);
        style = TEXT_STROKE;
    } else if (m_renderingStyle == STYLE_STROKE) {
        style |= TEXT_STROKE;
    }
    g->drawSubstring(str, offset, length, x, y + 1, anchor, style);
}

void SystemStringRendererIPhone::drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor) {
    g->setFont(m_font->canRenderString(str) ? m_font : m_fallbackFont);
    g->setColor(m_color);
    int style = TEXT_PLAIN;
    if (m_dropShadow) {
        g->setFontDropShadowParameters(m_shadowX, m_shadowY, m_shadowBlur, m_shadowColor);
        style = TEXT_SHADOW;
    }
    if (m_renderingStyle == STYLE_FILL_STROKE) {
        g->drawString(str, x, y + 1, anchor, style);
        g->setColor(m_strokeColor);
        style = TEXT_STROKE;
    } else if (m_renderingStyle == STYLE_STROKE) {
        style |= TEXT_STROKE;
    }
    g->drawString(str, x, y + 1, anchor, style);
}

void SystemStringRendererIPhone::drawString(midp::Graphics* g, midp::StringBuffer* buffer, int x, int y,
                                            int anchor) {
    midp::ReferenceCountedPointer<midp::String> str(buffer->toString());
    g->setFont(m_font->canRenderString(str) ? m_font : m_fallbackFont);
    g->setColor(m_color);
    int style = TEXT_PLAIN;
    if (m_dropShadow) {
        g->setFontDropShadowParameters(m_shadowX, m_shadowY, m_shadowBlur, m_shadowColor);
        style = TEXT_SHADOW;
    }
    if (m_renderingStyle == STYLE_FILL_STROKE) {
        g->drawString(buffer->toString(), x, y + 1, anchor, style);
        g->setColor(m_strokeColor);
        style = TEXT_STROKE;
    } else if (m_renderingStyle == STYLE_STROKE) {
        style |= TEXT_STROKE;
    }
    g->drawString(buffer->toString(), x, y + 1, anchor, style);
}

SystemStringRendererIPhone::~SystemStringRendererIPhone() {
    midp::DECREF(m_fallbackFont);
}

SystemStringRendererIPhone::SystemStringRendererIPhone(midp::Font* font, midp::Font* fallbackFont)
    : SystemStringRenderer(font),
      m_renderingStyle(STYLE_FILL),
      m_shadowX(0),
      m_shadowY(0),
      m_shadowBlur(0),
      m_strokeColor(WHITE),
      m_shadowColor(DEFAULT_SHADOW_COLOR),
      m_dropShadow(false),
      m_fallbackFont(fallbackFont) {
    midp::INCREF(fallbackFont);
}

void SystemStringRendererIPhone::setIPhoneDropShadowColor(int argb) {
    m_shadowColor = argb;
}
