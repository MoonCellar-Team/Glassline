#include "runtime/midp/Graphics.h"

#include "m3g/OpenGLES20Renderer.h"
#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/ShaderBinding.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/GLES20.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/ReferenceCounted.h"
#include "sdk/string.h"

void* operator new(size_t);

extern "C" {
void glDrawArrays(unsigned mode, int first, int count);
void glScissor(int x, int y, int width, int height);
void glTexParameteri(int target, int pname, int param);
void glVertexAttribPointer(unsigned index, int size, unsigned type, unsigned char normalized, int stride,
                            const void* pointer);
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
const unsigned kInv255Bits = 0x3b808081;

float degToRad() {
    union {
        unsigned bits;
        float value;
    } scale = {kDegToRadBits};
    return Math::PI * scale.value;
}

float inv255() {
    union {
        unsigned bits;
        float value;
    } scale = {kInv255Bits};
    return scale.value;
}

m3g::ShaderBinding* stockShader(int kind) {
    return static_cast<m3g::ShaderBinding*>(m3g::OpenGLES20ShaderSupport::getInstance()->getStockShader(
        static_cast<m3g::StockES2Shaders::StockES2Shaders>(kind)));
}

}

void GraphicsIPhoneGLES20::init() {
    m_needInit = false;
}

void GraphicsIPhoneGLES20::clipRect(int x, int y, int width, int height) {
    const int left = Math::max(m_unk24, x);
    const int right = Math::min(m_unk2c + m_unk24, width + x);
    const int top = Math::max(m_unk28, y);
    const int bottom = Math::min(m_unk30 + m_unk28, height + y);
    setClip(left, top, right - left, bottom - top);
}

void GraphicsIPhoneGLES20::copyArea(int, int, int, int, int, int, int) {}

void GraphicsIPhoneGLES20::drawArc(int x, int y, int width, int height, int, int arcAngle) {
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

void GraphicsIPhoneGLES20::drawChar(char, int, int, int) {}

void GraphicsIPhoneGLES20::drawChars(char*, int, int, int, int, int) {}

void GraphicsIPhoneGLES20::drawRGB(const int*, int, int, int, int, int, int, bool) {}

void GraphicsIPhoneGLES20::drawRoundRect(int, int, int, int, int, int) {}

void GraphicsIPhoneGLES20::drawString(String*, int, int, int, int) {}

void GraphicsIPhoneGLES20::drawSubstring(String*, int, int, int, int, int, int) {}

void GraphicsIPhoneGLES20::fillRoundRect(int, int, int, int, int, int) {}

int GraphicsIPhoneGLES20::getClipHeight() const {
    return m_unk30;
}

int GraphicsIPhoneGLES20::getClipWidth() const {
    return m_unk2c;
}

int GraphicsIPhoneGLES20::getClipX() const {
    return m_unk24;
}

int GraphicsIPhoneGLES20::getClipY() const {
    return m_unk28;
}

int GraphicsIPhoneGLES20::getDisplayColor(int color) const {
    return color;
}

int GraphicsIPhoneGLES20::getGrayScale() const {
    return 0;
}

int GraphicsIPhoneGLES20::getStrokeStyle() const {
    return 0;
}

void GraphicsIPhoneGLES20::setStrokeStyle(int) {}

void GraphicsIPhoneGLES20::bind2D() {
    m_bindPending = true;
}

m3g::Renderer* GraphicsIPhoneGLES20::createRenderer() {
    return new m3g::OpenGLES20Renderer(4, m_gl);
}

GraphicsIPhoneGLES20::~GraphicsIPhoneGLES20() {
    DECREF(m_display);
}

void GraphicsIPhoneGLES20::drawImage(const Image* image, int x, int y, int anchor) {
    drawRegion(image, 0, 0, image->getWidth(), image->getHeight(), 0, x, y, anchor);
}

void GraphicsIPhoneGLES20::drawLine(int x1, int y1, int x2, int y2) {
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
    glVertexAttribPointer(0, 2, kFloat, 0, 0, xy);
    m_gl->glEnableVertexAttribArray(0);
    glDrawArrays(kLines, 0, 2);
}

void GraphicsIPhoneGLES20::drawRect(int x, int y, int width, int height) {
    const float left = (float)x + 0.375f;
    const float top = (float)y + 0.375f;
    const float right = (float)width + left;
    const float bottom = (float)height + top;
    const float v[8] = {left, top, left, bottom, right, bottom, right, top};
    actualBind2D();
    disableTextures();
    glVertexAttribPointer(0, 2, kFloat, 0, 0, v);
    m_gl->glEnableVertexAttribArray(0);
    glDrawArrays(kLineLoop, 0, 4);
}

void GraphicsIPhoneGLES20::drawRegion(const Image* image, int srcX, int srcY, int width, int height,
                                      int transform, int x, int y, int anchor) {
    if (image == NULL)
        return;
    short destX = (short)x;
    if ((anchor & 2) == 0) {
        if ((anchor & 4) != 0)
            destX = (short)(destX - (short)image->getWidth());
    } else {
        destX = (short)(destX - (short)(image->getWidth() >> 1));
    }
    short destY = (short)y;
    if ((anchor & 0x10) == 0) {
        if ((anchor & 0x20) != 0)
            destY = (short)(destY - (short)image->getHeight());
    } else {
        destY = (short)(destY - (short)(image->getHeight() >> 1));
    }
    actualBind2D();
    enableTexture(static_cast<ImageIPhone*>(const_cast<Image*>(image)));
    const short left = transform == 2 ? (short)(destX + (short)width) : destX;
    const short right = transform == 2 ? destX : (short)(destX + (short)width);
    const short top = destY;
    const short bottom = (short)(destY + (short)height);
    struct Vert {
        short x;
        short y;
        short s;
        short t;
    };
    Vert v[4];
    const short xs[4] = {left, left, right, right};
    const short ys[4] = {top, bottom, top, bottom};
    const short ss[4] = {(short)srcX, (short)srcX, (short)(srcX + width), (short)(srcX + width)};
    const short ts[4] = {(short)srcY, (short)(srcY + height), (short)srcY, (short)(srcY + height)};
    for (int i = 0; i < 4; ++i) {
        v[i].x = (short)(m_xAxisX * xs[i] + m_yAxisX * ys[i] + m_originX);
        v[i].y = (short)(m_xAxisY * xs[i] + m_yAxisY * ys[i] + m_originY);
        v[i].s = ss[i];
        v[i].t = ts[i];
    }
    glVertexAttribPointer(0, 2, kShort, 0, 8, v);
    glVertexAttribPointer(5, 2, kShort, 0, 8, &v[0].s);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glEnableVertexAttribArray(5);
    glDrawArrays(kTriangleStrip, 0, 4);
    m_gl->glDisableVertexAttribArray(5);
}

void GraphicsIPhoneGLES20::fillArc(int x, int y, int width, int height, int startAngle, int arcAngle) {
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
    glVertexAttribPointer(0, 2, kFloat, 0, 0, pts);
    m_gl->glEnableVertexAttribArray(0);
    glDrawArrays(kTriangleFan, 0, count + 2);
    delete[] pts;
}

void GraphicsIPhoneGLES20::fillRect(int x, int y, int width, int height) {
#ifdef MEDECOMP_PORT
    Displayable* screen = m_display->getCurrent();
    const int extra = medecomp_wide_extra();
    if (extra != 0 && x == 0 && y == 0 && width == screen->getWidth() && height == screen->getHeight()) {
        x = -extra;
        width += 2 * extra;
    }
#endif
    const float left = (float)x + 0.375f;
    const float top = (float)y + 0.375f;
    const float right = (float)(width + x) + 0.375f;
    const float bottom = (float)(height + y) + 0.375f;
    const float v[8] = {left, top, left, bottom, right, top, right, bottom};
    actualBind2D();
    disableTextures();
    glVertexAttribPointer(0, 2, kFloat, 0, 0, v);
    m_gl->glEnableVertexAttribArray(0);
    glDrawArrays(kTriangleStrip, 0, 4);
}

void GraphicsIPhoneGLES20::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    const float v[6] = {(float)x1, (float)y1, (float)x2, (float)y2, (float)x3, (float)y3};
    actualBind2D();
    disableTextures();
    glVertexAttribPointer(0, 2, kFloat, 0, 0, v);
    m_gl->glEnableVertexAttribArray(0);
    glDrawArrays(kTriangleStrip, 0, 3);
}

void GraphicsIPhoneGLES20::setClip(int x, int y, int width, int height) {
    Displayable* screen = m_display->getCurrent();
    const int screenW = screen->getWidth();
    const int screenH = screen->getHeight();
#ifdef MEDECOMP_PORT
    const int extra = medecomp_wide_extra();
    if (extra != 0 && x == 0 && y == 0 && width == screenW && height == screenH && m_originX == 0 &&
        m_originY == 0 && m_xAxisX == 1 && m_yAxisY == 1) {
        x = -extra;
        width = screenW + 2 * extra;
    }
#endif
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
    const int orientation = m_display->getOrientation();
    int sx;
    int sy;
    if ((unsigned)(orientation - 2) < 2) {
        sx = (screenH - m_scissorY) - m_scissorH;
        sy = (screenW - m_scissorX) - m_scissorW;
#ifdef MEDECOMP_PORT
        sy += extra;
#endif
    } else if (orientation == 1) {
        sx = m_scissorX;
        sy = (screenH - m_scissorY) - m_scissorH;
    } else if (orientation == 0) {
        sx = (screenW - m_scissorX) - m_scissorW;
        sy = (screenH - m_scissorY) - m_scissorH;
    } else {
        return;
    }
#ifdef MEDECOMP_PORT
    glScissor(medecomp_px(sx), medecomp_px(sy), medecomp_px(sx + m_scissorH) - medecomp_px(sx),
              medecomp_px(sy + m_scissorW) - medecomp_px(sy));
#else
    glScissor(sx, sy, m_scissorH, m_scissorW);
#endif
}

void GraphicsIPhoneGLES20::translate(int x, int y) {
    const int oldX = getTranslateX();
    const int oldY = getTranslateY();
    m_transform.postTranslate((float)(-oldX), (float)(-oldY), 0.0f);
    Graphics::translate(x, y);
    m_transform.postTranslate((float)getTranslateX(), (float)getTranslateY(), 0.0f);
    m_transform.get(m_wvp);
    transpose(m_wvp);
    if (m_shader != NULL)
        m_gl->glUniformMatrix4fv(m_shader->location(0), 1, 0, m_wvp);
}

void GraphicsIPhoneGLES20::setColor(int red, int green, int blue, int alpha) {
    Graphics::setColor(red, green, blue, alpha);
    const float scale = inv255();
    m_color[0] = (float)red * scale;
    m_color[1] = (float)green * scale;
    m_color[2] = (float)blue * scale;
    m_color[3] = (float)alpha * scale;
    if (!g_invalidProgramState && m_shader != NULL && m_shader->location(2) != -1)
        m_gl->glUniform4fv(m_shader->location(2), 1, m_color);
}

int GraphicsIPhoneGLES20::actualBind2D() {
    if (!m_bindPending)
        return 0;
    m_bindPending = false;
    if (m_needInit)
        init();
    g_invalidProgramState = false;
#ifdef MEDECOMP_PORT
    const int fbHeight = 0x400 + 2 * medecomp_wide_extra();
    m_gl->glViewport(0, 0, medecomp_px(0x300), medecomp_px(fbHeight));
#else
    const int fbHeight = 0x400;
    m_gl->glViewport(0, 0, 0x300, 0x400);
#endif
    m_texture = NULL;
    m_texturesOn = true;
    disableTextures();
    m_gl->glDisable(0x0B44);
    m_gl->glDisable(0x0B71);
    m_gl->glDepthMask(0);
    m_gl->glColorMask(1, 1, 1, 1);
    m_gl->glBlendFunc(0x0302, 0x0303);
    m_gl->glEnable(0x0BE2);
    m_gl->glDisable(0x8037);
    const float proj[16] = {
        1.0f / 384.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f / (float)fbHeight, 0.0f, -1.0f,
        0.0f,          0.0f, -1.0f, 0.0f, 0.0f, 0.0f,           0.0f, 1.0f,
    };
    m3g::Transform projection;
    projection.set(proj);
    const int orientation = m_display->getOrientation();
    if (orientation == 1) {
        projection.postTranslate(384.0f, 512.0f, 0.0f);
        projection.postScale(1.0f, -1.0f, 1.0f);
    } else if ((unsigned)(orientation - 2) < 2) {
        projection.postTranslate(384.0f, (float)fbHeight * 0.5f, 0.0f);
        projection.postRotate(-90.0f, 0.0f, 0.0f, 1.0f);
        projection.postScale(1.0f, -1.0f, 1.0f);
        projection.postTranslate(-512.0f, -384.0f, 0.0f);
    }
    m_transform.set(&projection);
    m_gl->glEnable(0x0C11);
    Displayable* screen = m_display->getCurrent();
    setClip(0, 0, screen->getWidth(), screen->getHeight());
    translate(-getTranslateX(), -getTranslateY());
    return 1;
}

void GraphicsIPhoneGLES20::disableTextures() const {
    if (!m_texturesOn)
        return;
    m_shader = stockShader(4);
    m_gl->glUseProgram(m_shader->m_program);
    m_gl->glUniform4f(m_shader->location(2), m_color[0], m_color[1], m_color[2], m_color[3]);
    m_gl->glUniformMatrix4fv(m_shader->location(0), 1, 0, m_wvp);
    m_gl->glUniform4fv(m_shader->location(2), 1, m_color);
    m_texturesOn = false;
}

void GraphicsIPhoneGLES20::enableTexture(ImageIPhone* image) const {
    m_shader = stockShader(5);
    m_gl->glUseProgram(m_shader->m_program);
    m_gl->glUniformMatrix4fv(m_shader->location(0), 1, 0, m_wvp);
    m_gl->glActiveTexture(0x84c0);
    m_gl->glBlendFunc(1, 0x0303);
    if (m_texture != image) {
        m_texture = image;
        image->bindTexture();
#ifdef MEDECOMP_PORT
        const int filter = medecomp_render_scale() > 1.0f ? 0x2601 : 0x2600;
        glTexParameteri(kTexture2D, 0x2800, filter);
        glTexParameteri(kTexture2D, 0x2801, filter);
#else
        glTexParameteri(kTexture2D, 0x2800, 0x2600);
        glTexParameteri(kTexture2D, 0x2801, 0x2600);
#endif
        glTexParameteri(kTexture2D, 0x2802, 0x812f);
        glTexParameteri(kTexture2D, 0x2803, 0x812f);
        const float texW = (float)image->getTextureWidth();
        const float texH = (float)image->getTextureHeight();
        float m[16];
        memset(m, 0, sizeof(m));
        m[0] = 1.0f / texW;
        m[5] = 1.0f / texH;
        m[10] = 1.0f;
        m[12] = -((float)image->getTextureOffsetX() / texW);
        m[13] = -((float)image->getTextureOffsetY() / texH);
        m[15] = 1.0f;
        const int* tex = m_shader->texture(0);
        m_gl->glUniform1i(tex[0], 0);
        m_gl->glUniformMatrix4fv(tex[1], 1, 0, m);
    }
    m_texturesOn = true;
}

GraphicsIPhoneGLES20::GraphicsIPhoneGLES20(DisplayIPhone* display, GLES20* gl)
    : m_display(display),
      m_texture(NULL),
      m_texturesOn(false),
      m_bindPending(false),
      m_gl(gl),
      m_shader(NULL),
      m_needInit(true),
      m_wvp(),
      m_color(),
      m_transform(),
      m_scissorX(0),
      m_scissorY(0),
      m_scissorW(0),
      m_scissorH(0) {
    INCREF(m_display);
}

float* GraphicsIPhoneGLES20::getWVP() {
    return m_wvp;
}

}
