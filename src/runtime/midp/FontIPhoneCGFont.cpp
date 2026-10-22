#include "runtime/midp/FontIPhoneCGFont.h"

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "sdk/CoreGraphics.h"

extern "C" {

void CGFontRetain(CGFont* font);
void CGFontRelease(CGFont* font);
int CGFontGetUnitsPerEm(CGFont* font);
int CGFontGetAscent(CGFont* font);
int CGFontGetDescent(CGFont* font);
int CGFontGetLeading(CGFont* font);
void CGFontGetFontBBox(CGRect* bbox, CGFont* font);
void CGFontGetGlyphAdvances(CGFont* font, const unsigned short* glyphs, int count, int* advances);

int CFStringGetLength(const __CFString* string);
const unsigned short* CFStringGetCharactersPtr(const __CFString* string);

void CGContextSetFont(CGContext* context, CGFont* font);
void CGContextSetFontSize(CGContext* context, float size);
void CGContextSetTextMatrix(CGContext* context, float a, float b, float c, float d, float tx,
                            float ty);
void CGContextShowGlyphsAtPoint(CGContext* context, float x, float y, const unsigned short* glyphs,
                                int count);

}

namespace midp {

struct CFStringInlineBuffer {
    unsigned short buffer[64];
    const __CFString* theString;
    const unsigned short* directUniCharBuffer;
    int rangeLocation;
    int rangeLength;
    int bufferedRangeStart;
    int bufferedRangeEnd;
};

void FontIPhoneCGFont::getLayoutPadding(int& left, int& right, int& top, int& bottom) {
    left = 0;
    right = 0;
    top = m_padTop;
    bottom = m_padBottom;
}

int FontIPhoneCGFont::getBaselinePosition() const {
    return m_baseline;
}

int FontIPhoneCGFont::getHeight() const {
    return m_height;
}

bool FontIPhoneCGFont::canRenderChar(unsigned short ch) {
    return m_ttf->charToGlyph(ch) != 0;
}

bool FontIPhoneCGFont::canRenderString(String* str) {
    return m_ttf->canRenderString(str);
}

int FontIPhoneCGFont::getLeading() {
    return convertGlyphUnitToPixels(CGFontGetLeading(m_font));
}

int FontIPhoneCGFont::charWidth(unsigned short ch) const {
    unsigned short glyph = m_ttf->charToGlyph(ch);
    int advance = 0;
    CGFontGetGlyphAdvances(m_font, &glyph, 1, &advance);
    return convertGlyphUnitToPixels(advance);
}

FontIPhoneCGFont::~FontIPhoneCGFont() {
    CGFontRelease(m_font);
    DECREF(m_ttf);
    if (m_glyphs)
        delete[] m_glyphs;
    if (m_advances)
        delete[] m_advances;
}

int FontIPhoneCGFont::charsWidth(const unsigned short* chars, int offset, int length) const {
    ensureSpace(length);
    if (length > 0) {
        const unsigned short* text = chars + offset;
        unsigned short* glyph = m_glyphs;
        int left = length;
        do {
            *glyph = m_ttf->charToGlyph(*text);
            left = left - 1;
            text = text + 1;
            glyph = glyph + 1;
        } while (left != 0);
    }
    CGFontGetGlyphAdvances(m_font, m_glyphs, length, m_advances);
    int sum = 0;
    if (length > 0) {
        int* advance = m_advances;
        do {
            sum = convertGlyphUnitToPixels(*advance) + sum;
            length = length - 1;
            advance = advance + 1;
        } while (length != 0);
    }
    return sum;
}

int FontIPhoneCGFont::stringWidth(String* str) const {
    ReferenceCountedHardReference hold(str);
    int width = 0;
    const __CFString* cf = str->getCFString();
    if (cf) {
        int length = CFStringGetLength(cf);
        ensureSpace(length);
        CFStringInlineBuffer buffer;
        buffer.theString = cf;
        buffer.directUniCharBuffer = CFStringGetCharactersPtr(cf);
        buffer.rangeLocation = 0;
        buffer.rangeLength = length;
        buffer.bufferedRangeStart = 0;
        buffer.bufferedRangeEnd = 0;
        m_ttf->stringToGlyphs(&buffer, m_glyphs, length);
        CGFontGetGlyphAdvances(m_font, m_glyphs, length, m_advances);
        int units = 0;
        int left = length;
        int* advance = m_advances;
        while (left != 0) {
            left = left - 1;
            units = *advance + units;
            advance = advance + 1;
        }
        width = convertGlyphUnitToPixels(units);
    }
    return width;
}

int FontIPhoneCGFont::substringWidth(String* str, int offset, int length) {
    ReferenceCountedHardReference hold(str);
    int width = 0;
    const __CFString* cf = str->getCFString();
    if (cf) {
        int total = CFStringGetLength(cf);
        unsigned int count = (unsigned int)length;
        if ((unsigned int)(total - offset) < count)
            count = (unsigned int)(total - offset);
        ensureSpace((int)count);
        CFStringInlineBuffer buffer;
        buffer.theString = str->getCFString();
        buffer.directUniCharBuffer = CFStringGetCharactersPtr(cf);
        buffer.rangeLocation = offset;
        buffer.rangeLength = (int)count;
        buffer.bufferedRangeStart = 0;
        buffer.bufferedRangeEnd = 0;
        m_ttf->stringToGlyphs(&buffer, m_glyphs, (int)count);
        CGFontGetGlyphAdvances(m_font, m_glyphs, (int)count, m_advances);
        if ((int)count > 0) {
            int* advance = m_advances;
            unsigned int left = count;
            do {
                width = convertGlyphUnitToPixels(*advance) + width;
                left = left - 1;
                advance = advance + 1;
            } while (left != 0);
        }
    }
    return width;
}

void FontIPhoneCGFont::drawStringAtPoint(CGContext* context, CGPoint point,
                                        const __CFString* text) const {
    if (!text)
        return;
    int length = CFStringGetLength(text);
    ensureSpace(length);
    CFStringInlineBuffer buffer;
    buffer.theString = text;
    buffer.directUniCharBuffer = CFStringGetCharactersPtr(text);
    buffer.rangeLocation = 0;
    buffer.rangeLength = length;
    buffer.bufferedRangeStart = 0;
    buffer.bufferedRangeEnd = 0;
    m_ttf->stringToGlyphs(&buffer, m_glyphs, length);
    CGContextSetFont(context, m_font);
    CGContextSetTextMatrix(context, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    CGContextSetFontSize(context, m_size);
    CGContextShowGlyphsAtPoint(context, point.x, point.y + (float)m_baseline, m_glyphs, length);
}

FontIPhoneCGFont* FontIPhoneCGFont::deriveFont(float size) const {
    return new FontIPhoneCGFont(getCGFont(), size, m_ttf);
}

void FontIPhoneCGFont::ensureSpace(int count) const {
    if (count <= m_capacity)
        return;
    if (m_glyphs)
        delete[] m_glyphs;
    if (m_advances)
        delete[] m_advances;
    m_capacity = count;
    m_advances = new int[count];
    m_glyphs = new unsigned short[count];
}

FontIPhoneCGFont::FontIPhoneCGFont(CGFont* font, float size, FontIPhoneCGTTF* ttf)
    : FontIPhone(),
      m_font(font),
      m_size(size),
      m_height(0),
      m_padBottom(0),
      m_padTop(0),
      m_unitsToPixels(0.0f),
      m_glyphs(NULL),
      m_advances(NULL),
      m_capacity(0),
      m_baseline(0),
      m_ttf(ttf) {
    INCREF(ttf);
    CGFontRetain(m_font);
    m_unitsToPixels = size / (float)CGFontGetUnitsPerEm(m_font);
    int body = convertGlyphUnitToPixels(CGFontGetAscent(m_font) - CGFontGetDescent(m_font));
    CGRect bbox;
    CGFontGetFontBBox(&bbox, m_font);
    int bboxPx = convertGlyphUnitToPixels((int)bbox.size.height);
    int leadingPx = convertGlyphUnitToPixels(CGFontGetLeading(m_font));
    m_baseline = convertGlyphUnitToPixels(CGFontGetAscent(m_font));
    m_height = body + 2;
    int below = convertGlyphUnitToPixels(CGFontGetDescent(m_font) - CGFontGetLeading(m_font));
    m_padTop = below + leadingPx + bboxPx;
    int bottom = (leadingPx + bboxPx) - m_height - m_padTop;
    if (bottom < 0)
        bottom = 0;
    m_padBottom = bottom;
    ensureSpace(0x10);
}

CGFont* FontIPhoneCGFont::getCGFont() const {
    return m_font;
}

int FontIPhoneCGFont::convertGlyphUnitToPixels(int units) const {
    float scaled = (float)units * m_unitsToPixels;
    int truncated = (int)scaled;
    return truncated + ((float)truncated != scaled ? 1 : 0);
}

}
