#pragma once

#include "runtime/midp/Font.h"
#include "runtime/midp/ReferenceCounted.h"

struct UIFont;
struct CGFont;

namespace midp {

class FontIPhone : public Font {
public:
    FontIPhone();
    virtual ~FontIPhone();
};

struct CFStringInlineBuffer;

class DataInputStream;
class InputStream;

class FontIPhoneCGTTF : public ReferenceCounted {
public:
    struct EncodingRecord {};
    struct TableRecord {};

    struct GlyphMap {
        struct Node {
            Node* next;
            int key;
            int value;
        };
        GlyphMap();
        ~GlyphMap();
        int& operator[](const int& key);

        Node* m_head;
        char m_tail[0x14];
    };

    FontIPhoneCGTTF();
    virtual ~FontIPhoneCGTTF();
    static FontIPhoneCGTTF* createFromStream(InputStream* stream);
    bool readHeader(InputStream* stream);
    bool readCMapTable(DataInputStream& in, TableRecord& table);
    bool readEncodingRecord(DataInputStream& in, EncodingRecord& encoding, TableRecord& table);
    bool canRenderString(String* str);
    int charToGlyph(int ch);
    void stringToGlyphs(CFStringInlineBuffer* buffer, unsigned short* glyphs, int count);

    GlyphMap m_glyphs;
};

class FontIPhoneUIFont : public FontIPhone {
public:
    explicit FontIPhoneUIFont(UIFont* font);
    virtual ~FontIPhoneUIFont();
    virtual int charsWidth(const unsigned short* chars, int offset, int length) const;
    virtual int charWidth(unsigned short ch) const;
    virtual int getBaselinePosition() const;
    virtual int getHeight() const;
    virtual int stringWidth(String* str) const;
    virtual int substringWidth(String* str, int offset, int length);
    virtual int getLeading();
    virtual void drawStringAtPoint(CGContext* context, CGPoint point, const __CFString* text) const;
    Font* deriveFont(float size) const;
    UIFont* getUIFont() const;

    UIFont* m_font;
};

class FontIPhoneCGFont : public FontIPhone {
public:
    FontIPhoneCGFont(::CGFont* font, float size, FontIPhoneCGTTF* ttf);
    virtual ~FontIPhoneCGFont();

    virtual int charsWidth(const unsigned short* chars, int offset, int length) const;
    virtual int charWidth(unsigned short ch) const;
    virtual int getBaselinePosition() const;
    virtual int getHeight() const;
    virtual int stringWidth(String* str) const;
    virtual int substringWidth(String* str, int offset, int length);
    virtual int getLeading();
    virtual void getLayoutPadding(int& left, int& right, int& top, int& bottom);
    virtual bool canRenderString(String* str);
    virtual bool canRenderChar(unsigned short ch);
    virtual void drawStringAtPoint(CGContext* context, CGPoint point, const __CFString* text) const;

    ::CGFont* getCGFont() const;
    int convertGlyphUnitToPixels(int units) const;
    void ensureSpace(int count) const;
    FontIPhoneCGFont* deriveFont(float size) const;

    ::CGFont* m_font;
    float m_size;
    int m_height;
    int m_padBottom;
    int m_padTop;
    float m_unitsToPixels;
    mutable unsigned short* m_glyphs;
    mutable int* m_advances;
    mutable int m_capacity;
    int m_baseline;
    FontIPhoneCGTTF* m_ttf;
};

}
