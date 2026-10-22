#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

namespace midp {
class Graphics;
class String;
}

class WrappedString {
public:
    static const int MAX_LINES = 255;

    WrappedString();
    WrappedString(const WrappedString& other);
    virtual ~WrappedString();

    int getWrappedTextHeight();
    void drawLinesWithFont(midp::Graphics* g, int font, int x, int y, int anchor, int firstLine, int numLines);
    void drawWithFont(midp::Graphics* g, int font, int x, int y, int anchor);
    void draw(midp::Graphics* g, int x, int y, int anchor);
    void wrapSubString(midp::String* text, int font, int maxWidth, int start, int length);
    void wrapString(int textId, int font, int maxWidth);
    void wrapString(midp::String* text, int font, int maxWidth);
    int getNumWrappedLines();

    int m_font;
    midp::array<short> m_lineStart;
    midp::array<short> m_lineLength;
    int m_numLines;
    midp::ReferenceCountedPointer<midp::String> m_text;
};
