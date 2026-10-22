#include "game/WrappedString.h"

#include "game/AppEngine.h"
#include "game/StringRenderer.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"

static const int ANCHOR_TOP = 8;
static const int ANCHOR_VCENTER = 0x10;
static const int ANCHOR_BOTTOM = 0x20;

static const int NO_WORD = -1;

WrappedString::WrappedString()
    : m_font(0), m_lineStart(MAX_LINES), m_lineLength(MAX_LINES), m_numLines(0), m_text() {}

int WrappedString::getWrappedTextHeight() {
    StringRenderer* renderer = AppEngine::getCanvas()->getTextManager()->getStringRenderer(m_font);
    return renderer->getHeight() * m_numLines - renderer->getLeading();
}

void WrappedString::drawLinesWithFont(midp::Graphics* g, int font, int x, int y, int anchor, int firstLine,
                                      int numLines) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    StringRenderer* renderer = text->getStringRenderer(font);
    int clipTop = g->getClipY();
    int clipBottom = g->getClipHeight() + clipTop;
    int lineHeight = renderer->getHeight();
    int leading = renderer->getLeading();
    if (anchor & ANCHOR_VCENTER) {
        y -= (lineHeight * numLines - leading) >> 1;
        anchor = (anchor & ~ANCHOR_VCENTER) | ANCHOR_TOP;
    } else if (anchor & ANCHOR_BOTTOM) {
        y = y - lineHeight * numLines + leading;
        anchor = (anchor & ~ANCHOR_BOTTOM) | ANCHOR_TOP;
    }
    for (; numLines != 0; numLines--) {
        int start = m_lineStart[firstLine];
        int length = m_lineLength[firstLine];
        if (length > 0 && y < clipBottom && y + lineHeight > clipTop) {
            text->drawSubString(g, m_text, start, length, font, x, y, anchor);
        }
        firstLine++;
        y += lineHeight;
    }
}

void WrappedString::drawWithFont(midp::Graphics* g, int font, int x, int y, int anchor) {
    drawLinesWithFont(g, font, x, y, anchor, 0, m_numLines);
}

void WrappedString::draw(midp::Graphics* g, int x, int y, int anchor) {
    drawLinesWithFont(g, m_font, x, y, anchor, 0, m_numLines);
}

void WrappedString::wrapSubString(midp::String* text, int font, int maxWidth, int start, int length) {
    TextManager* textManager = AppEngine::getCanvas()->getTextManager();
    m_text = text;
    m_font = font;
    StringRenderer* renderer = textManager->getStringRenderer(font);
    int lineStart = start;
    int lastEnd = start - 1;
    m_numLines = 0;
    if (length != 0) {
        int wordStart = NO_WORD;
        int end = start + length;
        for (int i = start; i != end; i++) {
            unsigned short c = text->charAt(i);
            if (c == '\n' || c == '|') {
                m_lineStart[m_numLines] = lineStart;
                if (wordStart == NO_WORD) {
                    m_lineLength[m_numLines] = lastEnd - lineStart + 1;
                } else {
                    m_lineLength[m_numLines] = i - lineStart;
                }
                wordStart = NO_WORD;
                lastEnd = i;
                m_numLines++;
                lineStart = i + 1;
            } else if (c == ' ') {
                if (i != lineStart && wordStart != NO_WORD) {
                    wordStart = NO_WORD;
                    lastEnd = i - 1;
                }
            } else if (renderer->substringWidth(text, lineStart, i - lineStart + 1) <= maxWidth) {
                if (c == '-' && i != lineStart) {
                    lastEnd = i;
                    wordStart = NO_WORD;
                } else if (wordStart == NO_WORD) {
                    wordStart = i;
                }
            } else {
                m_lineStart[m_numLines] = lineStart;
                if (wordStart == NO_WORD) {
                    m_lineLength[m_numLines] = lastEnd - lineStart + 1;
                    lineStart = i;
                } else if (wordStart == lineStart) {
                    m_lineLength[m_numLines] = i - lineStart;
                    lineStart = i;
                } else {
                    m_lineLength[m_numLines] = lastEnd - lineStart + 1;
                    lineStart = wordStart;
                }
                lastEnd = lineStart;
                m_numLines++;
                wordStart = lineStart;
            }
        }
        if (wordStart != NO_WORD) {
            m_lineStart[m_numLines] = lineStart;
            m_lineLength[m_numLines] = end - lineStart;
            m_numLines++;
            return;
        }
        if (lineStart == lastEnd) {
            return;
        }
    }
    m_lineStart[m_numLines] = lineStart;
    m_lineLength[m_numLines] = lastEnd - lineStart + 1;
    m_numLines++;
}

void WrappedString::wrapString(int textId, int font, int maxWidth) {
    midp::String* text = AppEngine::getCanvas()->getTextManager()->getString(textId);
    wrapSubString(text, font, maxWidth, 0, text->length());
}

void WrappedString::wrapString(midp::String* text, int font, int maxWidth) {
    wrapSubString(text, font, maxWidth, 0, text->length());
}

WrappedString::~WrappedString() {}

WrappedString::WrappedString(const WrappedString& other)
    : m_font(other.m_font),
      m_lineStart(other.m_lineStart),
      m_lineLength(other.m_lineLength),
      m_numLines(other.m_numLines),
      m_text(other.m_text) {}

int WrappedString::getNumWrappedLines() {
    return m_numLines;
}
