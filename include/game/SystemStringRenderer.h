#pragma once

#include "game/StringRenderer.h"

namespace midp {
class Font;
}

class SystemStringRenderer : public StringRenderer {
public:
    explicit SystemStringRenderer(midp::Font* font);
    virtual ~SystemStringRenderer();
    virtual midp::Class* getClass() const;
    virtual void drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor);
    virtual void drawString(midp::Graphics* g, midp::StringBuffer* str, int x, int y, int anchor);
    virtual void drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length, int x, int y,
                               int anchor);
    virtual int charsWidth(const unsigned short* chars, int offset, int length) const;
    virtual int charWidth(unsigned short ch) const;
    virtual int getBaselinePosition() const;
    virtual int getHeight() const;
    virtual int getLeading() const;
    virtual int stringWidth(midp::String* str) const;
    virtual int substringWidth(midp::String* str, int offset, int length);
    void setColor(int color);

    midp::Font* m_font;
    int m_color;
};
