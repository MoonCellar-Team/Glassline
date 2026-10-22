#pragma once

#include "game/StringConstants.h"
#include "game/WrappedString.h"
#include "m3g/OpaqueObject.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"

namespace midp {
class Graphics;
class String;
class StringBuffer;
}
class StringRenderer;

class TextManager : public StringConstants {
public:
    static const int STRING_ID_BITS = 11;
    static const int FIRST_DYNAMIC_STRING = -12;
    static const int NUM_DYNAMIC_STRINGS = 10;
    static const int NUM_STRING_BUFFERS = 25;

    TextManager();
    virtual ~TextManager();
    void init();

    StringRenderer* getStringRenderer(int font) const;
    int getStringWidth(midp::String* text, int font) const;
    int getStringWidth(int textId, int font) const;
    int getBaseline(int font);
    int getLineHeight(int font) const;
    void drawSubString(midp::Graphics* g, midp::String* text, int start, int length, int font, int x, int y,
                       int anchor) const;
    void drawString(midp::Graphics* g, midp::String* text, int font, int x, int y, int anchor) const;
    void drawString(midp::Graphics* g, midp::StringBuffer* text, int font, int x, int y, int anchor) const;
    void drawString(midp::Graphics* g, int textId, int font, int x, int y, int anchor) const;

    midp::StringBuffer* clearStringBuffer();
    void appendStringToBuffer(midp::String* text);
    void appendStringToBuffer(const char* text);
    void appendIntToBuffer(midp::StringBuffer* buffer, int value);
    void appendIntToBufferWithThousandSep(int value);
    midp::String* convertIntToStringWithThousandSep(int value);
    void appendStringIdToBuffer(midp::StringBuffer* buffer, int textId);
    void appendMillisTimeToBuffer(midp::StringBuffer* buffer, int millis, int decimals);

    midp::String* replace(midp::String* format, midp::array<midp::ReferenceCountedPointer<midp::String>>& args);
    midp::String* replaceFirst(midp::String* format, midp::String* arg);

    int addStringsFile(midp::String* file);
    void unloadStrings();
    void loadStrings();
    int getLanguageCount();
    midp::String* getCurrentLocale();
    int getCurrentLanguage() const;
    void setCurrentLocale(midp::String* locale);
    void setCurrentLanguage(int language);
    midp::String* getLangString(int textId, int language);
    midp::String* getString(int textId) const;
    void dynamicString(int targetId, int formatId, midp::String* a, midp::String* b);
    void dynamicString(int targetId, int formatId, midp::String* a);
    void initStringBuffers();
    void initFonts();

    m3g::OpaqueObject<util::VectorSE<midp::String>, 0x18> m_stringFiles;
    m3g::OpaqueObject<util::VectorSE<StringRenderer>, 0x18> m_renderers;
    midp::array<midp::ReferenceCountedPointer<midp::String>> m_dynamicStrings;
    midp::array<midp::ReferenceCountedPointer<midp::String>> m_replaceArgs;
    midp::StringBuffer* m_buffer;
    midp::array<midp::ReferenceCountedPointer<midp::StringBuffer>> m_buffers;
    int m_bufferIndex;
    WrappedString m_wrappedString;
};
