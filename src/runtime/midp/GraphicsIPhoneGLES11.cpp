#include "runtime/midp/Graphics.h"

#include "m3g/OpenGLES11Renderer.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/GLES11.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "sdk/string.h"

void* operator new(size_t);

extern "C" {
void glColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void glDrawArrays(unsigned mode, int first, int count);
void glLoadIdentity();
void glLoadMatrixf(const float* m);
void glOrthof(float left, float right, float bottom, float top, float zNear, float zFar);
void glRotatef(float angle, float x, float y, float z);
void glScalef(float x, float y, float z);
void glScissor(int x, int y, int width, int height);
void glTexCoordPointer(int size, unsigned type, int stride, const void* pointer);
void glTexEnvi(int target, int pname, int param);
void glTexParameteri(int target, int pname, int param);
void glTranslatef(float x, float y, float z);
void glVertexPointer(int size, unsigned type, int stride, const void* pointer);
}

namespace midp {

namespace {

const int kTexture2D = 0x0DE1;
const int kFloat = 0x1406;
const int kShort = 0x1402;
const int kLines = 1;
const int kLineLoop = 2;
const int kTriangleStrip = 5;
const int kTriangleFan = 6;
const unsigned kDegToRadBits = 0x3bb60b61;

float degToRad() {
    union {
        unsigned bits;
        float value;
    } scale = {kDegToRadBits};
    return Math::PI * scale.value;
}

}

GraphicsIPhoneGLES11::GraphicsIPhoneGLES11(DisplayIPhone* display, GLES11* gl)
    : m_display(display),
      m_gl(gl),
      m_texture(NULL),
      m_texturesOn(false),
      m_bindPending(false),
      m_scissorX(0),
      m_scissorY(0),
      m_scissorW(0),
      m_scissorH(0) {
    INCREF(m_display);
}

void GraphicsIPhoneGLES11::clipRect(int x, int y, int width, int height) {
    const int left = Math::max(m_unk24, x);
    const int right = Math::min(m_unk2c + m_unk24, width + x);
    const int top = Math::max(m_unk28, y);
    const int bottom = Math::min(m_unk30 + m_unk28, height + y);
    setClip(left, top, right - left, bottom - top);
}

void GraphicsIPhoneGLES11::copyArea(int, int, int, int, int, int, int) {}

void GraphicsIPhoneGLES11::drawArc(int x, int y, int width, int height, int, int arcAngle) {
    if (arcAngle <= 0x167)
        return;
    const int x2 = width + x;
    const int y2 = height + y;
    const int xThird = width / 3 + x;
    const int yThird = height / 3 + y;
    const int xTwoThirds = (width << 1) / 3 + x;
    const int yTwoThirds = (height << 1) / 3 + y;
    drawLine(xThird, y, xTwoThirds, y);
    drawLine(xTwoThirds, y, x2, yThird);
    drawLine(x2, yThird, x2, yTwoThirds);
    drawLine(x2, yTwoThirds, xTwoThirds, y2);
    drawLine(xTwoThirds, y2, xThird, y2);
    drawLine(xThird, y2, x, yTwoThirds);
    drawLine(x, yTwoThirds, x, yThird);
    drawLine(x, yThird, xThird, y);
}

void GraphicsIPhoneGLES11::drawChar(char, int, int, int) {}

void GraphicsIPhoneGLES11::drawChars(char*, int, int, int, int, int) {}

void GraphicsIPhoneGLES11::drawRGB(const int*, int, int, int, int, int, int, bool) {}

void GraphicsIPhoneGLES11::drawRoundRect(int, int, int, int, int, int) {}

void GraphicsIPhoneGLES11::drawString(String* str, int, int, int, int) {
    ReferenceCountedHardReference hold(str);
}

void GraphicsIPhoneGLES11::drawSubstring(String* str, int, int, int, int, int, int) {
    ReferenceCountedHardReference hold(str);
}

void GraphicsIPhoneGLES11::fillRoundRect(int, int, int, int, int, int) {}

int GraphicsIPhoneGLES11::getClipHeight() const {
    return m_unk30;
}

int GraphicsIPhoneGLES11::getClipWidth() const {
    return m_unk2c;
}

int GraphicsIPhoneGLES11::getClipX() const {
    return m_unk24;
}

int GraphicsIPhoneGLES11::getClipY() const {
    return m_unk28;
}

int GraphicsIPhoneGLES11::getDisplayColor(int color) const {
    return color;
}

int GraphicsIPhoneGLES11::getGrayScale() const {
    return 0;
}

int GraphicsIPhoneGLES11::getStrokeStyle() const {
    return 0;
}

void GraphicsIPhoneGLES11::setStrokeStyle(int) {}

void GraphicsIPhoneGLES11::bind2D() {
    m_bindPending = true;
}

void GraphicsIPhoneGLES11::disableTextures() const {
    if (!m_texturesOn)
        return;
    int unit = 0x84c1;
    for (int i = 2; i != 0; --i) {
        m_gl->glActiveTexture(unit);
        m_gl->glClientActiveTexture(unit);
        --unit;
        m_gl->glDisableClientState(0x8078);
        m_gl->glDisable(kTexture2D);
    }
    m_texturesOn = false;
}

m3g::Renderer* GraphicsIPhoneGLES11::createRenderer() {
    return new m3g::OpenGLES11Renderer(2, m_gl);
}

GraphicsIPhoneGLES11::~GraphicsIPhoneGLES11() {
    DECREF(m_display);
}

void GraphicsIPhoneGLES11::drawImage(const Image* image, int x, int y, int anchor) {
    const int width = image->getWidth();
    const int height = image->getHeight();
    drawRegion(image, 0, 0, width, height, 0, x, y, anchor);
}

void GraphicsIPhoneGLES11::drawLine(int x1, int y1, int x2, int y2) {
    float leftX = (float)x1;
    float leftY = (float)y1;
    float rightX = (float)x2;
    float rightY = (float)y2;
    if ((float)x2 < (float)x1) {
        leftX = (float)x2;
        rightX = (float)x1;
        leftY = (float)y2;
        rightY = (float)y1;
    }
    rightX += 1.0f;
    if (rightY > leftY) {
        rightY += 1.0f;
    } else {
        const float raised = leftY + 1.0f;
        if (leftY <= rightY)
            rightY += 1.0f;
        leftY = raised;
    }
    const float xy[4] = {leftX, leftY, rightX, rightY};
    actualBind2D();
    disableTextures();
    glVertexPointer(2, kFloat, 0, xy);
    glDrawArrays(kLines, 0, 2);
}

void GraphicsIPhoneGLES11::drawRect(int x, int y, int width, int height) {
    const float left = (float)x + 0.375f;
    const float top = (float)y + 0.375f;
    const float right = (float)width + left;
    const float bottom = (float)height + top;
    const float ox = (float)m_originX;
    const float oy = (float)m_originY;
    const float xx = (float)m_xAxisX;
    const float xy = (float)m_xAxisY;
    const float yx = (float)m_yAxisX;
    const float yy = (float)m_yAxisY;
    const float v[8] = {
        ox + xx * left + yx * top,    oy + xy * left + yy * top,
        ox + xx * left + yx * bottom, oy + xy * left + yy * bottom,
        ox + xx * right + yx * bottom, oy + xy * right + yy * bottom,
        ox + xx * right + yx * top,   oy + xy * right + yy * top,
    };
    actualBind2D();
    disableTextures();
    glVertexPointer(2, kFloat, 0, v);
    glDrawArrays(kLineLoop, 0, 4);
}

void GraphicsIPhoneGLES11::drawRegion(const Image* image, int srcX, int srcY, int width, int height,
                                      int transform, int x, int y, int anchor) {
    if (image == NULL)
        return;
    int dx = 0;
    if ((anchor & 2) != 0)
        dx = width >> 1;
    else if ((anchor & 4) != 0)
        dx = width;
    int dy = 0;
    if ((anchor & 0x10) != 0)
        dy = height >> 1;
    else if ((anchor & 0x20) != 0)
        dy = height;
    const int destX = x - dx;
    const int destY = y - dy;
    actualBind2D();
    enableTexture(static_cast<ImageIPhone*>(const_cast<Image*>(image)));
    const int left = transform == 2 ? destX + width : destX;
    const int right = transform == 2 ? destX : destX + width;
    const int top = destY;
    const int bottom = destY + height;
    struct Vert {
        short x;
        short y;
        short s;
        short t;
    };
    Vert v[4];
    const int xs[4] = {left, left, right, right};
    const int ys[4] = {top, bottom, top, bottom};
    const int ss[4] = {srcX, srcX, srcX + width, srcX + width};
    const int ts[4] = {srcY, srcY + height, srcY, srcY + height};
    for (int i = 0; i < 4; ++i) {
        v[i].x = (short)(m_xAxisX * xs[i] + m_yAxisX * ys[i] + m_originX);
        v[i].y = (short)(m_xAxisY * xs[i] + m_yAxisY * ys[i] + m_originY);
        v[i].s = (short)ss[i];
        v[i].t = (short)ts[i];
    }
    glVertexPointer(2, kShort, 8, v);
    glTexCoordPointer(2, kShort, 8, &v[0].s);
    m_gl->glEnableClientState(0x8078);
    glDrawArrays(kTriangleStrip, 0, 4);
}

void GraphicsIPhoneGLES11::fillArc(int x, int y, int width, int height, int startAngle, int arcAngle) {
    const float rad = degToRad();
    const int span = Math::abs(arcAngle);
    int count = span;
    int step = 1;
    if (span >= 8) {
        count = span / 8;
        step = 8;
    }
    if (arcAngle < 0)
        step = -step;
    const float halfW = (float)(width / 2);
    const float halfH = (float)(height / 2);
    float* pts = new float[(count + 2) * 2];
    pts[0] = (float)x;
    pts[1] = (float)y;
    int angle = startAngle;
    for (int i = 0; i < count; ++i) {
        const float a = (float)angle * rad;
        pts[(i + 1) * 2] = (float)x + Math::cos(a) * halfW;
        pts[(i + 1) * 2 + 1] = (float)y + Math::sin(a) * halfH;
        angle += step;
    }
    const float end = (float)(arcAngle + startAngle) * rad;
    pts[(count + 1) * 2] = (float)x + Math::cos(end) * halfW;
    pts[(count + 1) * 2 + 1] = (float)y + Math::sin(end) * halfH;
    actualBind2D();
    disableTextures();
    glVertexPointer(2, kFloat, 0, pts);
    glDrawArrays(kTriangleFan, 0, count + 2);
    delete[] pts;
}

void GraphicsIPhoneGLES11::fillRect(int x, int y, int width, int height) {
    const float ox = (float)m_originX + 0.375f;
    const float oy = (float)m_originY + 0.375f;
    const float left = (float)x;
    const float top = (float)y;
    const float right = (float)(width + x);
    const float bottom = (float)(height + y);
    const float xx = (float)m_xAxisX;
    const float xy = (float)m_xAxisY;
    const float yx = (float)m_yAxisX;
    const float yy = (float)m_yAxisY;
    const float v[8] = {
        xx * left + ox + yx * top,      xy * left + oy + yy * top,
        xx * left + ox + yx * bottom,   xy * left + oy + yy * bottom,
        xx * right + ox + yx * top,     xy * right + oy + yy * top,
        xx * right + ox + yx * bottom,  xy * right + oy + yy * bottom,
    };
    actualBind2D();
    disableTextures();
    glVertexPointer(2, kFloat, 0, v);
    glDrawArrays(kTriangleStrip, 0, 4);
}

void GraphicsIPhoneGLES11::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    const float v[6] = {(float)x1, (float)y1, (float)x2, (float)y2, (float)x3, (float)y3};
    actualBind2D();
    disableTextures();
    glVertexPointer(2, kFloat, 0, v);
    glDrawArrays(kTriangleStrip, 0, 3);
}

void GraphicsIPhoneGLES11::setClip(int x, int y, int width, int height) {
    Displayable* screen = m_display->getCurrent();
    const int screenW = screen->getWidth();
    const int screenH = screen->getHeight();
    const int x0 = m_xAxisX * x + m_yAxisX * y + m_originX;
    const int y0 = m_xAxisY * x + m_yAxisY * y + m_originY;
    const int x1 = m_xAxisX * (width + x) + m_yAxisX * (height + y) + m_originX;
    const int y1 = m_xAxisY * (width + x) + m_yAxisY * (height + y) + m_originY;
    actualBind2D();
    m_unk24 = x;
    m_unk28 = y;
    m_unk2c = width;
    m_unk30 = height;
    m_scissorX = Math::min(x0, x1);
    m_scissorY = Math::min(y0, y1);
    m_scissorW = Math::abs(x0 - x1);
    m_scissorH = Math::abs(y0 - y1);
    if (m_scissorX <= 0 && m_scissorY <= 0 && m_scissorW >= screenW && m_scissorH >= screenH) {
        m_gl->glDisable(0x0C11);
        return;
    }
    m_gl->glEnable(0x0C11);
    const int orientation = m_display->getOrientation();
    int sx;
    int sy;
    if ((unsigned)(orientation - 2) < 2) {
        sx = (screenH - m_scissorY) - m_scissorH;
        sy = (screenW - m_scissorX) - m_scissorW;
    } else if (orientation == 1) {
        sx = x;
        sy = (screenH - m_scissorY) - m_scissorH;
    } else if (orientation == 0) {
        sx = (screenW - m_scissorX) - m_scissorW;
        sy = (screenH - m_scissorY) - m_scissorH;
    } else {
        return;
    }
    glScissor(sx, sy, m_scissorH, m_scissorW);
}

void GraphicsIPhoneGLES11::translate(int x, int y) {
    const int oldX = getTranslateX();
    const int oldY = getTranslateY();
    glTranslatef((float)(-oldX), (float)(-oldY), 0.0f);
    Graphics::translate(x, y);
    glTranslatef((float)getTranslateX(), (float)getTranslateY(), 0.0f);
}

void GraphicsIPhoneGLES11::setColor(int red, int green, int blue, int alpha) {
    Graphics::setColor(red, green, blue, alpha);
    glColor4ub((unsigned char)(red & 0xff), (unsigned char)(green & 0xff),
               (unsigned char)(blue & 0xff), (unsigned char)(alpha & 0xff));
}

int GraphicsIPhoneGLES11::actualBind2D() {
    if (!m_bindPending)
        return 0;
    m_bindPending = false;
    m_gl->glViewport(0, 0, 0x300, 0x400);
    m_gl->glDisableClientState(0x8075);
    m_gl->glDisableClientState(0x8076);
    m_gl->glEnableClientState(0x8074);
    DECREF(m_texture);
    m_texture = NULL;
    m_texturesOn = true;
    disableTextures();
    m_gl->glDisable(0x0B50);
    m_gl->glDisable(0x0B44);
    m_gl->glDisable(0x0B71);
    m_gl->glDepthMask(0);
    m_gl->glColorMask(1, 1, 1, 1);
    m_gl->glEnable(0x0BC0);
    m_gl->glBlendFunc(0x0302, 0x0303);
    m_gl->glEnable(0x0BE2);
    m_gl->glDisable(0x8037);
    m_gl->glMatrixMode(0x1701);
    glLoadIdentity();
    glOrthof(0.0f, 768.0f, 0.0f, 1024.0f, -1.0f, 1.0f);
    const int orientation = m_display->getOrientation();
    if (orientation == 1) {
        glTranslatef(384.0f, 512.0f, 0.0f);
        glScalef(1.0f, -1.0f, 1.0f);
    } else if ((unsigned)(orientation - 2) < 2) {
        glTranslatef(384.0f, 512.0f, 0.0f);
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        glScalef(1.0f, -1.0f, 1.0f);
        glTranslatef(-512.0f, -384.0f, 0.0f);
    }
    m_gl->glMatrixMode(0x1702);
    glLoadIdentity();
    m_gl->glMatrixMode(0x1700);
    glLoadIdentity();
    m_gl->glEnable(0x0C11);
    Displayable* screen = m_display->getCurrent();
    setClip(0, 0, screen->getWidth(), screen->getHeight());
    translate(-getTranslateX(), -getTranslateY());
    return 1;
}

void GraphicsIPhoneGLES11::enableTexture(ImageIPhone* image) const {
    m_gl->glActiveTexture(0x84c0);
    m_gl->glClientActiveTexture(0x84c0);
    m_gl->glEnable(kTexture2D);
    m_gl->glBlendFunc(1, 0x0303);
    if (m_texture != image || image->isInvalid()) {
        INCREF(image);
        DECREF(m_texture);
        m_texture = image;
        image->bindTexture();
        glTexEnvi(0x2300, 0x2200, 0x1e01);
        glTexParameteri(kTexture2D, 0x2800, 0x2600);
        glTexParameteri(kTexture2D, 0x2801, 0x2600);
        glTexParameteri(kTexture2D, 0x2802, 0x812f);
        glTexParameteri(kTexture2D, 0x2803, 0x812f);
        const float texW = (float)image->getTextureWidth();
        const float texH = (float)image->getTextureHeight();
        const float sx = 1.0f / texW;
        const float sy = 1.0f / texH;
        const float tx = -((float)image->getTextureOffsetX() / texW);
        const float ty = -((float)image->getTextureOffsetY() / texH);
        float m[16];
        memset(m, 0, sizeof(m));
        m[0] = sx;
        m[5] = sy;
        m[10] = 1.0f;
        m[12] = tx;
        m[13] = ty;
        m[15] = 1.0f;
        m_gl->glMatrixMode(0x1702);
        glLoadMatrixf(m);
        m_gl->glMatrixMode(0x1700);
    }
    m_texturesOn = true;
}

}
