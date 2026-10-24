#include "game/TextManager.h"

#include <ctype.h>
#include <stddef.h>

#include "game/AppEngine.h"
#include "game/CachedStringRenderer.h"
#include "game/LocaleManager.h"
#include "game/ResourceManager.h"
#include "game/StringRenderer.h"
#include "game/SystemStringRendererIPhone.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Font.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#ifdef MEDECOMP_PORT
#include "port/strings.h"
#endif

static const int RESOURCE_FONTS = 228;

static const int TEXT_TIME_SEPARATOR = 0x80b;
static const int TEXT_DECIMAL_SEPARATOR = 0x80c;
static const int TEXT_THOUSAND_SEPARATOR = 0x810;

static const int NUM_REPLACE_ARGS = 3;
static const int STRING_BUFFER_CAPACITY = 100;

static const int MILLIS_PER_SECOND = 1000;
static const int MILLIS_PER_MINUTE = 60000;
static const int MILLIS_PER_HOUR = 3600000;

typedef midp::array<midp::ReferenceCountedPointer<midp::String>> StringArray;

TextManager::TextManager()
    : m_stringFiles(),
      m_renderers(),
      m_dynamicStrings(NUM_DYNAMIC_STRINGS),
      m_replaceArgs(NUM_REPLACE_ARGS),
      m_buffer(NULL),
      m_buffers(),
      m_bufferIndex(0),
      m_wrappedString() {}

StringRenderer* TextManager::getStringRenderer(int font) const {
    return m_renderers.get()->elementAt(font);
}

int TextManager::getStringWidth(midp::String* text, int font) const {
    return getStringRenderer(font)->stringWidth(text);
}

int TextManager::getBaseline(int font) {
    return getStringRenderer(font)->getBaselinePosition();
}

int TextManager::getLineHeight(int font) const {
    return getStringRenderer(font)->getHeight();
}

void TextManager::drawSubString(midp::Graphics* g, midp::String* text, int start, int length, int font, int x,
                                int y, int anchor) const {
    getStringRenderer(font)->drawSubstring(g, text, start, length, x, y, anchor);
}

void TextManager::drawString(midp::Graphics* g, midp::String* text, int font, int x, int y, int anchor) const {
    getStringRenderer(font)->drawString(g, text, x, y, anchor);
}

void TextManager::drawString(midp::Graphics* g, midp::StringBuffer* text, int font, int x, int y,
                             int anchor) const {
    drawString(g, text->toString(), font, x, y, anchor);
}

void TextManager::appendStringToBuffer(midp::String* text) {
    m_buffer->append(text);
}

midp::StringBuffer* TextManager::clearStringBuffer() {
    m_bufferIndex++;
    if (m_bufferIndex >= NUM_STRING_BUFFERS) {
        m_bufferIndex = 0;
    }
    m_buffer = m_buffers[m_bufferIndex];
    m_buffer->setLength(0);
    return m_buffer;
}

void TextManager::appendIntToBuffer(midp::StringBuffer* buffer, int value) {
    if (value == 0) {
        buffer->append('0');
        return;
    }
    if (value < 0) {
        value = -value;
        buffer->append('-');
    }
    bool started = false;
    int divisor = 1000000000;
    for (int i = 0; i < 10; i++) {
        int digit = value / divisor;
        if (digit != 0 || started) {
            buffer->append((unsigned short)('0' + digit));
            started = true;
        }
        value -= digit * divisor;
        divisor /= 10;
    }
}

void TextManager::appendStringToBuffer(const char* text) {
    m_buffer->append(text);
}

void TextManager::initStringBuffers() {
    m_buffers = midp::array<midp::ReferenceCountedPointer<midp::StringBuffer>>(NUM_STRING_BUFFERS);
    for (int i = 0; i < NUM_STRING_BUFFERS; i++) {
        m_buffers[i] = new midp::StringBuffer(STRING_BUFFER_CAPACITY);
    }
    m_buffer = NULL;
    m_bufferIndex = 0;
}

midp::String* TextManager::replace(midp::String* format, StringArray& args) {
    int length = format->length();
    midp::StringBuffer* buffer = clearStringBuffer();
    bool escaped = false;
    for (int i = 0; i < length; i++) {
        unsigned short c = format->charAt(i);
        if (escaped && isnumber(c)) {
            if (c == '\\') {
                buffer->append('\\');
            } else {
                buffer->append(args[c - '0']);
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            buffer->append(c);
        }
    }
    return buffer->toString();
}

midp::String* TextManager::replaceFirst(midp::String* format, midp::String* arg) {
    m_replaceArgs[0] = arg;
    return replace(format, m_replaceArgs);
}

void TextManager::initFonts() {
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_FONTS));
    int count = in.readInt();
    midp::Font* font = NULL;
    for (int i = 0; i < count; i++) {
        midp::ReferenceCountedPointer<midp::String> systemName = in.readUTF();
        midp::ReferenceCountedPointer<midp::String> fileName = in.readUTF();
        float size = in.readFloat();
        int style = in.readInt();
        int shadow = in.readInt();
        int shadowDx = in.readInt();
        int shadowDy = in.readInt();
        int shadowBlur = in.readInt();
        int color = in.readInt();
        int strokeColor = in.readInt();
        int shadowColor = in.readInt();
        int cacheSize = in.readInt();
        if (systemName && systemName->length() > 0) {
            font = midp::Font::getFont(systemName, size);
        } else if (fileName && fileName->length() > 0) {
            font = midp::Font::getFont(midp::Class::getResourceAsStream(fileName), size);
        }
        midp::Font* fallbackFont = midp::Font::getFont(new midp::String("Helvetica-BoldOblique"), size);
        int renderingStyle;
        if (style == 2) {
            renderingStyle = 3;
        } else if (style == 1) {
            renderingStyle = 2;
        } else {
            renderingStyle = 1;
        }
        SystemStringRendererIPhone* renderer = new SystemStringRendererIPhone(font, fallbackFont);
        renderer->setIPhoneRenderingStyle(renderingStyle);
        renderer->setStrokeColor(strokeColor);
        renderer->useIPhoneFontDropShadow(shadow != 0);
        renderer->setIPhoneDropShadowParameters(shadowDx, shadowDy, shadowBlur);
        renderer->setIPhoneDropShadowColor(shadowColor);
        renderer->setColor(color);
        m_renderers.get()->addElement(new CachedStringRenderer(renderer, cacheSize));
    }
}

int TextManager::addStringsFile(midp::String* file) {
    m_stringFiles.get()->addElement(file);
    return m_stringFiles.get()->size();
}

void TextManager::unloadStrings() {
    LocaleManager* locales = LocaleManager::getInstance();
    StringArray empty(0);
    int count = m_stringFiles.get()->size();
    for (int i = 0; i < count; i++) {
        midp::ReferenceCountedPointer<midp::String> file = m_stringFiles.get()->elementAt(i);
        locales->setStringPool(i + 1, empty, NULL);
    }
}

void TextManager::loadStrings() {
    LocaleManager* locales = LocaleManager::getInstance();
    int count = m_stringFiles.get()->size();
    for (int i = 0; i < count; i++) {
        midp::ReferenceCountedPointer<midp::String> file = m_stringFiles.get()->elementAt(i);
        StringArray strings = locales->loadStrings(file, NULL);
#ifdef MEDECOMP_PORT
        port::applyStringOverrides(i + 1, strings, NULL);
#endif
        locales->setStringPool(i + 1, strings, NULL);
    }
}

int TextManager::getLanguageCount() {
    return LocaleManager::getInstance()->getSupportedLocales().m_length;
}

midp::String* TextManager::getCurrentLocale() {
    return LocaleManager::getInstance()->getLocale();
}

int TextManager::getCurrentLanguage() const {
    LocaleManager* locales = LocaleManager::getInstance();
    StringArray supported = locales->getSupportedLocales();
    midp::ReferenceCountedPointer<midp::String> locale = locales->getLocale();
    for (int i = 0; i < supported.m_length; i++) {
        if (locale->equals(supported[i])) {
            return i;
        }
    }
    return -1;
}

void TextManager::setCurrentLocale(midp::String* locale) {
    LocaleManager* locales = LocaleManager::getInstance();
    StringArray supported = locales->getSupportedLocales();
    if (getCurrentLanguage() >= 0) {
        unloadStrings();
    }
    locales->setLocale(locale);
    loadStrings();
}

void TextManager::setCurrentLanguage(int language) {
#ifdef MEDECOMP_PORT
    if (language < 0 || language >= getLanguageCount())
        return;
#endif
    if (getCurrentLanguage() != language) {
        StringArray supported = LocaleManager::getInstance()->getSupportedLocales();
        setCurrentLocale(supported[language]);
    }
}

midp::String* TextManager::getLangString(int textId, int language) {
    LocaleManager* locales = LocaleManager::getInstance();
    StringArray supported = locales->getSupportedLocales();
    midp::String* s = locales->getString(textId, supported[language]);
#ifdef MEDECOMP_PORT
    s = port::withInputHints(textId, s, supported[language]);
#endif
    return s;
}

midp::String* TextManager::getString(int textId) const {
    midp::String* s;
    if (textId < FIRST_DYNAMIC_STRING + NUM_DYNAMIC_STRINGS) {
        s = m_dynamicStrings[textId - FIRST_DYNAMIC_STRING];
    } else {
        s = LocaleManager::getInstance()->getString(textId, NULL);
    }
#ifdef MEDECOMP_PORT
    s = port::withInputHints(textId, s, NULL);
#endif
    return s;
}

void TextManager::appendIntToBufferWithThousandSep(int value) {
    midp::StringBuffer* buffer = m_buffer;
    if (value == 0) {
        buffer->append('0');
        return;
    }
    if (value < 0) {
        value = -value;
        buffer->append('-');
    }
    bool started = false;
    int divisor = 1000000000;
    for (int i = 0; i < 10; i++) {
        int digit = value / divisor;
        if (digit != 0 || started) {
            buffer->append((unsigned short)('0' + digit));
            if (divisor == 1000 || divisor == 1000000 || divisor == 1000000000) {
                buffer->append(getString(TEXT_THOUSAND_SEPARATOR));
            }
            started = true;
        }
        value -= digit * divisor;
        divisor /= 10;
    }
}

midp::String* TextManager::convertIntToStringWithThousandSep(int value) {
    midp::StringBuffer* buffer = clearStringBuffer();
    appendIntToBufferWithThousandSep(value);
    return buffer->toString();
}

void TextManager::appendStringIdToBuffer(midp::StringBuffer* buffer, int textId) {
    buffer->append(getString(textId));
}

void TextManager::appendMillisTimeToBuffer(midp::StringBuffer* buffer, int millis, int decimals) {
    int time = midp::Math::abs(millis);
    if (millis < 0) {
        buffer->append('-');
    }
    if (time >= MILLIS_PER_HOUR) {
        if (time < 10 * MILLIS_PER_HOUR) {
            buffer->append('0');
        }
        appendIntToBuffer(buffer, time / MILLIS_PER_HOUR);
        appendStringIdToBuffer(buffer, TEXT_TIME_SEPARATOR);
    }
    int minutes = time / MILLIS_PER_MINUTE % 60;
    if (minutes < 10) {
        buffer->append('0');
    }
    int seconds = time / MILLIS_PER_SECOND % 60;
    appendIntToBuffer(buffer, minutes);
    appendStringIdToBuffer(buffer, TEXT_TIME_SEPARATOR);
    if (seconds < 10) {
        buffer->append('0');
    }
    appendIntToBuffer(buffer, seconds);
    if (decimals > 0) {
        int fraction = time % MILLIS_PER_SECOND;
        appendStringIdToBuffer(buffer, TEXT_DECIMAL_SEPARATOR);
        int digits[3] = {fraction / 100, fraction / 10 % 10, fraction % 10};
        int count = midp::Math::min(decimals, 3);
        for (int i = 0; i != count; i++) {
            buffer->append(digits[i]);
        }
    }
}

void TextManager::dynamicString(int targetId, int formatId, midp::String* a, midp::String* b) {
    midp::String* format = getString(formatId);
    StringArray args = m_replaceArgs;
    args[0] = a;
    args[1] = b;
    m_dynamicStrings[targetId - FIRST_DYNAMIC_STRING] = replace(format, args);
}

void TextManager::dynamicString(int targetId, int formatId, midp::String* a) {
    midp::String* format = getString(formatId);
    m_dynamicStrings[targetId - FIRST_DYNAMIC_STRING] = replaceFirst(format, a);
}

int TextManager::getStringWidth(int textId, int font) const {
    StringRenderer* renderer = getStringRenderer(font);
    return renderer->stringWidth(getString(textId));
}

void TextManager::drawString(midp::Graphics* g, int textId, int font, int x, int y, int anchor) const {
    drawString(g, getString(textId), font, x, y, anchor);
}

void TextManager::init() {
    LocaleManager* locales = LocaleManager::getInstance();
    locales->setStringIdBits(STRING_ID_BITS);
    StringArray supported = locales->getSupportedLocales();
    midp::ReferenceCountedPointer<midp::String> headerFile = new midp::String("strings_hdr.bin");
    for (int i = 0; i < supported.m_length; i++) {
        StringArray strings = locales->loadStrings(headerFile, supported[i]);
#ifdef MEDECOMP_PORT
        port::applyStringOverrides(0, strings, supported[i]);
#endif
        locales->setStringPool(0, strings, supported[i]);
    }
    addStringsFile(new midp::String("strings_generic.bin"));
    loadStrings();
    initStringBuffers();
    initFonts();
}

TextManager::~TextManager() {}
