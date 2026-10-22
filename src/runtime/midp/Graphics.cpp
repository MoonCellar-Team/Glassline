#include "runtime/midp/Graphics.h"

#include "runtime/midp/Font.h"
#include "runtime/midp/GraphicsClass.h"
#include "runtime/midp/ReferenceCounted.h"

namespace midp {

Class* Graphics::getClass() const {
    return new GraphicsClass();
}

int Graphics::getAlphaComponent() const {
    return m_alpha;
}

int Graphics::getBlueComponent() const {
    return m_blue;
}

int Graphics::getColor() const {
    return (m_alpha << 24) | (m_red << 16) | (m_green << 8) | m_blue;
}

Font* Graphics::getFont() const {
    return m_font;
}

int Graphics::getGreenComponent() const {
    return m_green;
}

int Graphics::getRedComponent() const {
    return m_red;
}

int Graphics::getTranslateX() const {
    return m_translateX;
}

int Graphics::getTranslateY() const {
    return m_translateY;
}

void Graphics::setColor(int rgb) {
    setColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

void Graphics::setColor(int red, int green, int blue) {
    setColor(red, green, blue, 255);
}

void Graphics::setColor(int red, int green, int blue, int alpha) {
    m_red = red & 0xff;
    m_green = green & 0xff;
    m_blue = blue & 0xff;
    m_alpha = alpha & 0xff;
}

void Graphics::setGrayScale(int gray) {
    setColor(gray, gray, gray, 255);
}

void Graphics::translate(int x, int y) {
    m_translateX += x;
    m_translateY += y;
}

void Graphics::setFont(Font* font) {
    if (font == NULL)
        font = Font::getDefaultFont();
    INCREF(font);
    DECREF(m_font);
    m_font = font;
}

Graphics::~Graphics() {
    DECREF(m_font);
}

Graphics::Graphics()
    : m_red(255), m_green(255), m_blue(255), m_alpha(255),
      m_translateX(0), m_translateY(0), m_font(NULL) {
    setFont(NULL);
}

void Graphics::setFontDropShadowParameters(int, int, int, int) {}

}
