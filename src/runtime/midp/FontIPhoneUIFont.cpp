#include "runtime/midp/FontIPhone.h"

#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

extern "C" {

struct __CFString;
typedef const __CFString* CFStringRef;

void* objc_msgSend(void* self, const char* sel, ...);
void objc_msgSend_stret(void* result, void* self, const char* sel, ...);
CFStringRef CFStringCreateWithFormat(void* alloc, void* formatOptions, CFStringRef format, ...);
void UIGraphicsPushContext(CGContext* context);
void UIGraphicsPopContext(void);

}

void* operator new(size_t);

namespace midp {

static float msgFloat(void* obj, const char* sel) {
    union {
        void* p;
        float f;
    } bits;
    bits.p = objc_msgSend(obj, sel);
    return bits.f;
}

static int textWidth(const __CFString* text, UIFont* font) {
    CGSize size;
    objc_msgSend_stret(&size, const_cast<__CFString*>(text), "sizeWithFont:", font);
    return static_cast<int>(size.width);
}

int FontIPhoneUIFont::charsWidth(const unsigned short*, int, int) const {
    return 0;
}

int FontIPhoneUIFont::getLeading() {
    float leading = msgFloat(m_font, "leading");
    float descender = msgFloat(m_font, "descender");
    float ascender = msgFloat(m_font, "ascender");
    return static_cast<int>(leading + (descender - ascender));
}

int FontIPhoneUIFont::getHeight() const {
    return static_cast<int>(msgFloat(m_font, "leading"));
}

int FontIPhoneUIFont::getBaselinePosition() const {
    return static_cast<int>(msgFloat(m_font, "ascender"));
}

int FontIPhoneUIFont::substringWidth(String* str, int offset, int length) {
    ReferenceCountedHardReference hold(str);
    ReferenceCountedPointer<String> part(str->substring(offset, offset + length));
    return textWidth(part->getCFString(), m_font);
}

int FontIPhoneUIFont::stringWidth(String* str) const {
    ReferenceCountedHardReference hold(str);
    return textWidth(str->getCFString(), m_font);
}

int FontIPhoneUIFont::charWidth(unsigned short ch) const {
    CFStringRef text = CFStringCreateWithFormat(0, 0, CFSTR("%C"), ch);
    int width = textWidth(text, m_font);
    CFRelease(text);
    return width;
}

void FontIPhoneUIFont::drawStringAtPoint(CGContext* context, CGPoint point, const __CFString* text) const {
    UIGraphicsPushContext(context);
    CGSize unused;
    objc_msgSend_stret(&unused, const_cast<__CFString*>(text), "drawAtPoint:withFont:", point.x, point.y, getUIFont());
    UIGraphicsPopContext();
}

FontIPhoneUIFont::~FontIPhoneUIFont() {
    objc_msgSend(m_font, "release");
}

Font* FontIPhoneUIFont::deriveFont(float size) const {
    UIFont* scaled = static_cast<UIFont*>(objc_msgSend(getUIFont(), "fontWithSize:", size));
    return new FontIPhoneUIFont(scaled);
}

FontIPhoneUIFont::FontIPhoneUIFont(UIFont* font) : m_font(font) {
    objc_msgSend(m_font, "retain");
}

UIFont* FontIPhoneUIFont::getUIFont() const {
    return m_font;
}

}
