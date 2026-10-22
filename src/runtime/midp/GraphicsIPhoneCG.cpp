#include "runtime/midp/Graphics.h"

#include "runtime/midp/Font.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "sdk/UIKit.h"

namespace midp {

void makeArcPath(CGContext* context, int x, int y, int width, int height, int startAngle, int arcAngle);

namespace {

const float kInv255 = 0.003921568859368563f;

CGPoint CGPointMake(float x, float y) {
    CGPoint point;
    point.x = x;
    point.y = y;
    return point;
}

CGSize CGSizeMake(float width, float height) {
    CGSize size;
    size.width = width;
    size.height = height;
    return size;
}

CGRect CGRectMake(float x, float y, float width, float height) {
    CGRect rect;
    rect.origin.x = x;
    rect.origin.y = y;
    rect.size.width = width;
    rect.size.height = height;
    return rect;
}

void applyAnchor(int& x, int& y, int width, int height, int anchor) {
    if ((anchor & 4) != 0)
        x -= width;
    if ((anchor & 2) != 0)
        x -= width >> 1;
    if ((anchor & 0x20) != 0)
        y -= height;
    if ((anchor & 0x10) != 0)
        y -= height >> 1;
}

}

GraphicsIPhoneCG::GraphicsIPhoneCG(CGContext* context)
    : m_context(context),
      m_clipX(0),
      m_clipY(0),
      m_clipWidth(0),
      m_clipHeight(0),
      m_shadowDx(0),
      m_shadowDy(0),
      m_shadowBlur(0),
      m_shadowArgb(0) {
    CGContextRetain(m_context);
    CGContextSaveGState(m_context);
    CGContextSetBlendMode(m_context, 0);
    CGContextSetInterpolationQuality(m_context, 1);
    CGContextSetAllowsAntialiasing(m_context, false);
}

void GraphicsIPhoneCG::clipRect(int x, int y, int width, int height) {
    const int clipX = m_clipX;
    const int left = Math::max(clipX, x);
    const int right = Math::min(m_clipWidth + clipX, width + x);
    const int clipY = m_clipY;
    const int top = Math::max(clipY, y);
    const int bottom = Math::min(m_clipHeight + clipY, height + y);
    setClip(left, top, right - left, bottom - top);
}

void GraphicsIPhoneCG::copyArea(int, int, int, int, int, int, int) {}

void GraphicsIPhoneCG::drawChar(char, int, int, int) {}

void GraphicsIPhoneCG::drawChars(char*, int, int, int, int, int) {}

void GraphicsIPhoneCG::drawRGB(const int*, int, int, int, int, int, int, bool) {}

void GraphicsIPhoneCG::drawRoundRect(int, int, int, int, int, int) {}

void GraphicsIPhoneCG::setFontDropShadowParameters(int dx, int dy, int blur, int argb) {
    m_shadowDx = dx;
    m_shadowDy = dy;
    m_shadowBlur = blur;
    m_shadowArgb = argb;
}

void GraphicsIPhoneCG::fillRoundRect(int, int, int, int, int, int) {}

int GraphicsIPhoneCG::getClipHeight() const {
    return m_clipHeight;
}

int GraphicsIPhoneCG::getClipWidth() const {
    return m_clipWidth;
}

int GraphicsIPhoneCG::getClipX() const {
    return m_clipX;
}

int GraphicsIPhoneCG::getClipY() const {
    return m_clipY;
}

int GraphicsIPhoneCG::getDisplayColor(int color) const {
    return color;
}

int GraphicsIPhoneCG::getGrayScale() const {
    return 0;
}

int GraphicsIPhoneCG::getStrokeStyle() const {
    return 0;
}

void GraphicsIPhoneCG::setStrokeStyle(int) {}

void GraphicsIPhoneCG::bind2D() {}

void GraphicsIPhoneCG::setColor(int red, int green, int blue, int alpha) {
    Graphics::setColor(red, green, blue, alpha);
    const float r = static_cast<float>(red) * kInv255;
    const float g = static_cast<float>(green) * kInv255;
    const float b = static_cast<float>(blue) * kInv255;
    const float a = static_cast<float>(alpha) * kInv255;
    CGContextSetRGBFillColor(m_context, r, g, b, a);
    CGContextSetRGBStrokeColor(m_context, r, g, b, a);
}

void GraphicsIPhoneCG::setClip(int x, int y, int width, int height) {
    m_clipX = x;
    m_clipY = y;
    m_clipWidth = width;
    m_clipHeight = height;
    CGContextClipToRect(m_context, CGRectMake(static_cast<float>(x), static_cast<float>(y),
                                              static_cast<float>(width), static_cast<float>(height)));
}

void GraphicsIPhoneCG::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    CGContextBeginPath(m_context);
    CGContextMoveToPoint(m_context, static_cast<float>(x1), static_cast<float>(y1));
    CGContextAddLineToPoint(m_context, static_cast<float>(x2), static_cast<float>(y2));
    CGContextAddLineToPoint(m_context, static_cast<float>(x3), static_cast<float>(y3));
    CGContextFillPath(m_context);
}

void GraphicsIPhoneCG::fillRect(int x, int y, int width, int height) {
    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);
    const float bottom = static_cast<float>(height + y);
    const float right = static_cast<float>(width + x);
    CGContextBeginPath(m_context);
    CGContextMoveToPoint(m_context, fx, fy);
    CGContextAddLineToPoint(m_context, fx, bottom);
    CGContextAddLineToPoint(m_context, right, bottom);
    CGContextAddLineToPoint(m_context, right, fy);
    CGContextFillPath(m_context);
}

void GraphicsIPhoneCG::fillArc(int x, int y, int width, int height, int startAngle, int arcAngle) {
    makeArcPath(m_context, x, y, width, height, startAngle, arcAngle);
    if (arcAngle < 360) {
        CGContextAddLineToPoint(m_context, static_cast<float>(x) + static_cast<float>(width) * 0.5f,
                                static_cast<float>(y) + static_cast<float>(height) * 0.5f);
    }
    CGContextFillPath(m_context);
}

void GraphicsIPhoneCG::drawSubstring(String* str, int offset, int length, int x, int y, int anchor,
                                     int style) {
    ReferenceCountedHardReference hold(str);
    if (str->length() != 0) {
        String* part = str->substring(offset, length + offset);
        drawString(part, x, y, anchor, style);
    }
}

void GraphicsIPhoneCG::drawString(String* str, int x, int y, int anchor, int style) {
    ReferenceCountedHardReference hold(str);
    if (str->length() == 0)
        return;
    Font* font = getFont();
    const int width = font->stringWidth(str);
    int dx = 0;
    if ((anchor & 2) != 0)
        dx = -(width >> 1);
    if ((anchor & 4) != 0)
        dx = -width;
    CGContextSetAllowsAntialiasing(m_context, true);
    UIGraphicsPushContext(m_context);
    const CGPoint point = CGPointMake(static_cast<float>(dx + x), static_cast<float>(y));
    if ((style & 1) != 0) {
        void* colorSpace = CGColorSpaceCreateDeviceRGB();
        const unsigned argb = static_cast<unsigned>(m_shadowArgb);
        float components[4];
        components[0] = static_cast<float>((argb & 0xffffff) >> 16) * kInv255;
        components[1] = static_cast<float>((argb & 0xffff) >> 8) * kInv255;
        components[2] = static_cast<float>(argb & 0xff) * kInv255;
        components[3] = static_cast<float>(argb >> 24) * kInv255;
        void* color = CGColorCreate(colorSpace, components);
        const CGSize offset = CGSizeMake(static_cast<float>(m_shadowDx), static_cast<float>(-m_shadowDy));
        CGContextSetShadowWithColor(m_context, offset, static_cast<float>(m_shadowBlur), color);
        CGColorRelease(color);
        CGColorSpaceRelease(colorSpace);
    }
    CGContextSetTextDrawingMode(m_context, (style & 2) != 0 ? 1 : 0);
    font->drawStringAtPoint(m_context, point, str->getCFString());
    UIGraphicsPopContext();
    CGContextSetAllowsAntialiasing(m_context, false);
}

void GraphicsIPhoneCG::drawRegion(const Image* image, int srcX, int srcY, int width, int height,
                                  int transform, int x, int y, int anchor) {
    applyAnchor(x, y, width, height, anchor);
    if (transform != 0)
        return;
    CGImageRef full = static_cast<const ImageIPhone*>(image)->getCGImageRef();
#ifdef MEDECOMP_PORT
    const float k = static_cast<float>(static_cast<const ImageIPhone*>(image)->m_scale);
    CGImageRef part = CGImageCreateWithImageInRect(
        full, CGRectMake(static_cast<float>(srcX) * k, static_cast<float>(srcY) * k, static_cast<float>(width) * k,
                         static_cast<float>(height) * k));
#else
    CGImageRef part = CGImageCreateWithImageInRect(
        full, CGRectMake(static_cast<float>(srcX), static_cast<float>(srcY), static_cast<float>(width),
                         static_cast<float>(height)));
#endif
    CGContextDrawImage(m_context, CGRectMake(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height)),
                       part);
    CGImageRelease(part);
}

void GraphicsIPhoneCG::drawRect(int x, int y, int width, int height) {
    CGContextStrokeRect(m_context, CGRectMake(static_cast<float>(x), static_cast<float>(y),
                                              static_cast<float>(width), static_cast<float>(height)));
}

void GraphicsIPhoneCG::drawLine(int x1, int y1, int x2, int y2) {
    CGContextBeginPath(m_context);
    CGContextMoveToPoint(m_context, static_cast<float>(x1), static_cast<float>(y1));
    CGContextAddLineToPoint(m_context, static_cast<float>(x2), static_cast<float>(y2));
    CGContextStrokePath(m_context);
}

void GraphicsIPhoneCG::drawArc(int x, int y, int width, int height, int startAngle, int arcAngle) {
    makeArcPath(m_context, x, y, width, height, startAngle, arcAngle);
    CGContextStrokePath(m_context);
}

void GraphicsIPhoneCG::drawImage(const Image* image, int x, int y, int anchor) {
    const int width = image->getWidth();
    const int height = image->getHeight();
    applyAnchor(x, y, width, height, anchor);
    CGImageRef cgImage = static_cast<const ImageIPhone*>(image)->getCGImageRef();
    CGImageRetain(cgImage);
    CGContextDrawImage(m_context, CGRectMake(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height)),
                       cgImage);
    CGImageRelease(cgImage);
}

GraphicsIPhoneCG::~GraphicsIPhoneCG() {
    CGContextRestoreGState(m_context);
    CGContextRelease(m_context);
    m_context = NULL;
}

}
