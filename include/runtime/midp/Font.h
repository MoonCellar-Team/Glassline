#pragma once

#include "runtime/midp/Object.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "sdk/CoreGraphics.h"

struct __CFString;

namespace midp {

class Class;
class InputStream;
class String;

class Font : public Object {
public:
    Font();
    virtual ~Font();
    static Font* getFont(InputStream* in, float size);
    static Font* getFont(ReferenceCountedPointer<String> name, float size);
    virtual int charsWidth(const unsigned short* chars, int offset, int length) const = 0;
    virtual int charWidth(unsigned short ch) const = 0;
    virtual int getBaselinePosition() const = 0;
    virtual int getHeight() const = 0;
    virtual int stringWidth(String* str) const = 0;
    virtual int substringWidth(String* str, int offset, int length) = 0;
    virtual int getLeading() = 0;
    virtual void getLayoutPadding(int& left, int& right, int& top, int& bottom);
    virtual bool canRenderString(String* str);
    virtual bool canRenderChar(unsigned short ch);
    virtual void drawStringAtPoint(CGContext* context, CGPoint point, const __CFString* text) const = 0;
    virtual Class* getClass() const;
    static Font* getDefaultFont();
};

}
