#include "game/UI/DisplayNameEntry.h"

#include "game/AppEngine.h"
#include "game/MonkeyApp.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

static const int TEXT_PROMPT = 0x928;
static const int TEXT_INPUT = 0x929;
static const int TEXT_BACK = 0x82f;
static const int TEXT_OK = 0x82e;
static const int FONT_BODY = 2;
static const int ANCHOR_PROMPT = 9;
static const int ANCHOR_NAME = 10;
static const int NAME_LIMIT = 0x19;
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

bool DisplayNameEntry::pointerPressed(int x, int y, int id) {
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

bool DisplayNameEntry::pointerDragged(int x, int y, int id) {
    (void)id;
    if (m_okButton.isPressed() && !m_okButton.contains(x, y)) {
        m_okButton.unpress();
    }
    if (m_backButton.isPressed() && !m_backButton.contains(x, y)) {
        m_backButton.unpress();
    }
    return false;
}

void DisplayNameEntry::update(int dt) {
    (void)dt;
#ifdef MEDECOMP_PORT
    if (isClosed()) {
        return;
    }
    s_caretMs += dt;
    int key = 0;
    std::string typed = medecomp_line(this, NAME_LIMIT, &key);
    if (typed != s_typed) {
        s_typed = typed;
        s_caretMs = 0;
        m_name = new midp::String(typed.c_str());
        if (m_name->length() > 0) {
            int backW = m_backButton.getWidth();
            int backH = m_backButton.getHeight();
            int okW = m_okButton.getWidth();
            m_okButton.setPosition((m_width - 0x10 - backW) - okW, (m_height - 5) - backH);
        } else {
            m_okButton.setPosition(OFFSCREEN, OFFSCREEN);
        }
    }
    if (key == KEY_RETURN && m_name->length() > 0) {
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
    AppEngine::getCanvas()->getWindowStore().setWaitingForMainThread();
    MonkeyApp* app = static_cast<MonkeyApp*>(AppEngine::getCanvas()->getMIDlet());
    m_name = app->getInputString(title, NAME_LIMIT);
    AppEngine::getCanvas()->getWindowStore().unsetWaitingForMainThread();
    if (static_cast<midp::String*>(m_name) != NULL) {
        if (m_name->length() > NAME_LIMIT) {
            m_name = m_name->substring(0, NAME_LIMIT);
        }
        if (m_name->length() > 0) {
            int backW = m_backButton.getWidth();
            int backH = m_backButton.getHeight();
            int okW = m_okButton.getWidth();
            m_okButton.setPosition((m_width - 0x10 - backW) - okW, (m_height - 5) - backH);
        }
    }
    m_editing = false;
}

void DisplayNameEntry::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_frame.render(g, originY, originX);
    int frameX = m_frame.getX();
    int frameY = m_frame.getY();
    m_prompt.draw(g, frameX + 0x14, frameY + 0x19, ANCHOR_PROMPT);
    int promptH = m_prompt.getWrappedTextHeight();
    frameX = m_frame.getX();
    frameY = m_frame.getY();
    int frameW = m_frame.getWidth();
    int lineH = text->getLineHeight(FONT_BODY);
    g->setColor(0);
    g->drawRect(frameX + 0x14, promptH + frameY + 0x2d, frameW - 0x28, lineH + 6);
    text->drawString(g, m_name, FONT_BODY, m_width >> 1, promptH + frameY + 0x30, ANCHOR_NAME);
#ifdef MEDECOMP_PORT
    if (!isClosed()) {
        drawCaret(g, m_name, m_width >> 1, promptH + frameY + 0x30);
    }
#endif
    m_okButton.render(g, originY, originX);
    m_backButton.render(g, originY, originX);
}

bool DisplayNameEntry::pointerReleased(int x, int y, int id) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int promptH = m_prompt.getWrappedTextHeight();
    int frameX = m_frame.getX();
    int frameY = m_frame.getY();
    int frameW = m_frame.getWidth();
    int lineH = text->getLineHeight(FONT_BODY);
    int top = promptH + frameY + 0x2d;
    if (frameX + 0x14 < x && x < frameX + frameW - 0x14 && top < y && y < top + lineH + 6) {
        m_editing = true;
    }
    if (m_okButton.contains(x, y)) {
        m_okButton.pointerReleased(x, y, id);
        const __CFString* cf = m_name->getCFString();
        const char* name = CFStringGetCStringPtr(cf, kCFStringEncodingUTF8);
        SceneGame* scene = 0;
        if (name == NULL) {
            char* buffer = new char[0x65];
            CFStringGetCString(cf, buffer, 100, kCFStringEncodingUTF8);
            scene = AppEngine::getCanvas()->getSceneGame();
            scene->m_displayName = buffer;
            delete buffer;
        } else {
            scene = AppEngine::getCanvas()->getSceneGame();
            scene->m_displayName = name;
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

DisplayNameEntry::DisplayNameEntry()
    : m_frame(0, 0, 1, 1),
      m_name(NULL),
      m_backButton(TEXT_BACK),
      m_okButton(TEXT_OK),
      m_editing(false) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_prompt.wrapString(TEXT_PROMPT, FONT_BODY, m_width - 0x84);
    int frameH = m_prompt.getWrappedTextHeight() + text->getLineHeight(FONT_BODY) + 0x4c;
    m_frame.setPosition(0x2e, (m_height - frameH) >> 1);
    m_frame.setDimensions(m_width - 0x5c, frameH);
    int backW = m_backButton.getWidth();
    int backH = m_backButton.getHeight();
    m_backButton.setPosition((m_width - 8) - backW, (m_height - 5) - backH);
    m_okButton.setPosition(OFFSCREEN, OFFSCREEN);
    m_name = new midp::String("");
}

DisplayNameEntry::~DisplayNameEntry() {
#ifdef MEDECOMP_PORT
    medecomp_line_close(this);
#endif
}

}
