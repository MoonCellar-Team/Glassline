#pragma once

#include "game/StringRenderer.h"

class StringRecord;

class CachedStringRenderer : public StringRenderer {
public:
    CachedStringRenderer(StringRenderer* renderer, int size);
    virtual ~CachedStringRenderer();
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
    void bufferedDrawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor);

    StringRenderer* m_renderer;
    StringRecord* m_records;
    short* m_order;
    int m_size;
};
