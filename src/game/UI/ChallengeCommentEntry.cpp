#include "game/UI/ChallengeCommentEntry.h"

#include <algorithm>

#include "game/AppEngine.h"
#include "game/MonkeyApp.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

static const int TEXT_PROMPT = 0x961;
static const int TEXT_INPUT = 0x929;
static const int TEXT_BACK = 0x82f;
static const int TEXT_OK = 0x82e;
static const int FONT_BODY = 2;
static const int ANCHOR_PROMPT = 9;
static const int ANCHOR_COMMENT = 10;
static const int COMMENT_LIMIT = 0xff;
static const int OFFSCREEN = -3000;

#ifdef MEDECOMP_PORT
#include <string>

extern "C" const char* medecomp_line(const void* owner, int limit, int* key);
extern "C" void medecomp_line_close(const void* owner);
static const int KEY_RETURN = 13;
static const int KEY_ESCAPE = 27;
static const int CARET_BLINK_MS = 500;
static std::string s_typed;
static int s_caretMs;

static void drawCaret(midp::Graphics* g, midp::String* line, int centerX, int y) {
    if ((s_caretMs / CARET_BLINK_MS) % 2 != 0) {
        return;
    }
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int width = line != NULL ? text->getStringWidth(line, FONT_BODY) : 0;
    g->setColor(0);
    g->fillRect(centerX + width / 2 + 2, y, 2, text->getLineHeight(FONT_BODY));
}

#define TAP_BUTTON(button) \
    pointerReleased((button).getX() + (button).getWidth() / 2, (button).getY() + (button).getHeight() / 2, 0)
#endif

namespace UI {

bool ChallengeCommentEntry::pointerPressed(int x, int y, int id) {
    if (m_okButton.contains(x, y)) {
        m_okButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

bool ChallengeCommentEntry::pointerDragged(int x, int y, int id) {
    (void)id;
    if (m_okButton.isPressed() && !m_okButton.contains(x, y)) {
        m_okButton.unpress();
    }
    if (m_backButton.isPressed() && !m_backButton.contains(x, y)) {
        m_backButton.unpress();
    }
    return false;
}

void ChallengeCommentEntry::update(int dt) {
    (void)dt;
#ifdef MEDECOMP_PORT
    if (isClosed()) {
        return;
    }
    s_caretMs += dt;
    int key = 0;
    std::string typed = medecomp_line(this, COMMENT_LIMIT, &key);
    if (m_text == NULL || typed != s_typed) {
        s_typed = typed;
        s_caretMs = 0;
        m_text = new midp::String(typed.c_str());
        if (m_text->length() > 0) {
            int backW = m_backButton.getWidth();
            int backH = m_backButton.getHeight();
            int okW = m_okButton.getWidth();
            m_okButton.setPosition((m_width - 0x10 - backW) - okW, (m_height - 5) - backH);
        } else {
            m_okButton.setPosition(OFFSCREEN, OFFSCREEN);
        }
        m_comment.wrapString(m_text, FONT_BODY, m_width - 0xac);
        updateDimensions();
    }
    if (key == KEY_RETURN && m_text->length() > 0) {
        TAP_BUTTON(m_okButton);
    } else if (key == KEY_ESCAPE) {
        TAP_BUTTON(m_backButton);
    }
    return;
#endif
    if (!m_editing) {
        return;
    }
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> title(text->getString(TEXT_INPUT));
    MonkeyApp* app = static_cast<MonkeyApp*>(AppEngine::getCanvas()->getMIDlet());
    m_text = app->getInputString(title, COMMENT_LIMIT);
    if (m_text->length() > COMMENT_LIMIT) {
        m_text = m_text->substring(0, COMMENT_LIMIT);
    }
    if (m_text->length() > 0) {
        int backW = m_backButton.getWidth();
        int backH = m_backButton.getHeight();
        int okW = m_okButton.getWidth();
        m_okButton.setPosition((m_width - 0x10 - backW) - okW, (m_height - 5) - backH);
    }
    m_comment.wrapString(m_text, FONT_BODY, m_width - 0xac);
    updateDimensions();
    m_editing = false;
}

void ChallengeCommentEntry::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_frame.render(g, originY, originX);
    int frameX = m_frame.getX();
    int frameY = m_frame.getY();
    m_prompt.draw(g, frameX + 0x28, frameY + 0x19, ANCHOR_PROMPT);
    int promptH = m_prompt.getWrappedTextHeight();
    frameX = m_frame.getX();
    frameY = m_frame.getY();
    int frameW = m_frame.getWidth();
    int boxH = std::max(m_comment.getWrappedTextHeight(), text->getLineHeight(FONT_BODY));
    g->setColor(0);
    g->drawRect(frameX + 0x28, promptH + frameY + 0x2d, frameW - 0x50, boxH + 6);
    if (m_comment.getNumWrappedLines() > 0) {
        m_comment.draw(g, m_width >> 1, promptH + frameY + 0x30, ANCHOR_COMMENT);
    }
#ifdef MEDECOMP_PORT
    if (!isClosed()) {
        int lines = m_comment.getNumWrappedLines();
        midp::ReferenceCountedPointer<midp::String> last(NULL);
        if (lines > 0) {
            int start = m_comment.m_lineStart[lines - 1];
            last = m_comment.m_text->substring(start, start + m_comment.m_lineLength[lines - 1]);
        }
        int lastY = promptH + frameY + 0x30 + (lines > 0 ? lines - 1 : 0) * text->getLineHeight(FONT_BODY);
        drawCaret(g, last, m_width >> 1, lastY);
    }
#endif
    m_okButton.render(g, originY, originX);
    m_backButton.render(g, originY, originX);
}

bool ChallengeCommentEntry::pointerReleased(int x, int y, int id) {
    AppEngine* engine = AppEngine::getCanvas();
    TextManager* text = engine->getTextManager();
    int promptH = m_prompt.getWrappedTextHeight();
    int frameX = m_frame.getX();
    int frameY = m_frame.getY();
    int frameW = m_frame.getWidth();
    int boxH = std::max(m_comment.getWrappedTextHeight(), text->getLineHeight(FONT_BODY));
    int top = promptH + frameY + 0x2d;
    if (frameX + 0x28 < x && x < frameX + frameW - 0x28 && top < y && y < top + boxH + 6) {
        m_editing = true;
    }
    if (m_okButton.contains(x, y)) {
        m_okButton.pointerReleased(x, y, id);
        if (m_text != NULL) {
            const __CFString* cf = m_text->getCFString();
            const char* comment = CFStringGetCStringPtr(cf, CFStringGetSystemEncoding());
            engine->getSceneGame()->m_challengeComment = comment;
        }
        close(RESULT_OK);
        return true;
    }
    if (m_okButton.isPressed()) {
        m_okButton.unpress();
    }
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerReleased(x, y, id);
        close(RESULT_CANCEL);
        return true;
    }
    if (m_backButton.isPressed()) {
        m_backButton.unpress();
    }
    return false;
}

void ChallengeCommentEntry::updateDimensions() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int width = m_width;
    m_prompt.wrapString(TEXT_PROMPT, FONT_BODY, width - 0xac);
    int promptH = m_prompt.getWrappedTextHeight();
    int boxH = std::max(m_comment.getWrappedTextHeight(), text->getLineHeight(FONT_BODY));
    int frameH = promptH + boxH + 0x4c;
    m_frame.setPosition(0x2e, (m_height - frameH) >> 1);
    m_frame.setDimensions(width - 0x5c, frameH);
}

ChallengeCommentEntry::ChallengeCommentEntry()
    : m_frame(0, 0, 1, 1),
      m_text(NULL),
      m_backButton(TEXT_BACK),
      m_okButton(TEXT_OK),
      m_editing(false) {
    updateDimensions();
    int backW = m_backButton.getWidth();
    int backH = m_backButton.getHeight();
    m_backButton.setPosition((m_width - 8) - backW, (m_height - 5) - backH);
    m_okButton.setPosition(OFFSCREEN, OFFSCREEN);
}

ChallengeCommentEntry::~ChallengeCommentEntry() {
#ifdef MEDECOMP_PORT
    medecomp_line_close(this);
#endif
}

}
