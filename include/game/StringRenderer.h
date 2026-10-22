#pragma once

#include "runtime/midp/Object.h"

namespace midp {
class Class;
class Graphics;
class String;
class StringBuffer;
}

class StringRenderer : public midp::Object {
public:
    StringRenderer();
    virtual ~StringRenderer();
    virtual midp::Class* getClass() const = 0;
    virtual void drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor) = 0;
    virtual void drawString(midp::Graphics* g, midp::StringBuffer* str, int x, int y,
                            int anchor) = 0;
    virtual void drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length, int x, int y,
                               int anchor) = 0;
    virtual int charsWidth(const unsigned short* chars, int offset, int length) const = 0;
    virtual int charWidth(unsigned short ch) const = 0;
    virtual int getBaselinePosition() const = 0;
    virtual int getHeight() const = 0;
    virtual int getLeading() const = 0;
    virtual int stringWidth(midp::String* str) const = 0;
    virtual int substringWidth(midp::String* str, int offset, int length) = 0;
    virtual void getStringTexturePadding(int& left, int& right, int& top, int& bottom);
};
