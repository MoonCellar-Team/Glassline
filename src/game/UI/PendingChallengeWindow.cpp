#include "game/UI/PendingChallengeWindow.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int TEXT_MESSAGE = 0x967;
static const int TEXT_MORE = 0x966;
static const int TEXT_ACCEPT = 0x968;
static const int TEXT_DECLINE = 0x969;
static const int TEXT_LATER = 0x96a;
static const int FONT_BODY = 2;
static const int ANCHOR_BLOCK = 10;
static const int ANCHOR_MORE = 9;

namespace UI {

static const WindowResult RESULT_LATER = static_cast<WindowResult>(3);

const std::string& PendingChallengeWindow::getChallengeID() const {
    return m_challengeId;
}

bool PendingChallengeWindow::pointerPressed(int x, int y, int id) {
    MajorButton* button = 0;
    if (m_later.contains(x, y)) {
        button = &m_later;
    } else if (m_decline.contains(x, y)) {
        button = &m_decline;
    } else if (m_accept.contains(x, y)) {
        button = &m_accept;
    } else {
        return false;
    }
    button->pointerPressed(x, y, id);
    return true;
}

bool PendingChallengeWindow::pointerDragged(int x, int y, int id) {
    (void)id;
    if (m_later.isPressed() && !m_later.contains(x, y)) {
        m_later.unpress();
    }
    if (m_decline.isPressed() && !m_decline.contains(x, y)) {
        m_decline.unpress();
    }
    if (m_accept.isPressed() && !m_accept.contains(x, y)) {
        m_accept.unpress();
    }
    return false;
}

void PendingChallengeWindow::render(midp::Graphics* g, int originY, int originX) {
    AppEngine* engine = AppEngine::getCanvas();
    TextManager* text = engine->getTextManager();
    int more = engine->getPendingChallengeCount() - 1;
    midp::ReferenceCountedPointer<midp::String> moreText(NULL);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendIntToBuffer(buffer, more);
    midp::String* format = text->getString(TEXT_MORE);
    moreText = text->replaceFirst(format, buffer->toString());
    m_frame.render(g, originY, originX);
    int midX = m_width >> 1;
    int frameY = m_frame.getY();
    int nameH = m_name.getWrappedTextHeight();
    int frameX = m_frame.getX();
    int frameW = m_frame.getWidth();
    int moreW = text->getStringWidth(moreText, FONT_BODY);
    int messageH = m_message.getWrappedTextHeight();
    m_name.draw(g, midX, frameY + 10, ANCHOR_BLOCK);
    int messageY = frameY + nameH + 0x1e;
    m_message.draw(g, midX, messageY, ANCHOR_BLOCK);
    if (more >= 1) {
        int moreX = (frameX + frameW - 10) - moreW;
        text->drawString(g, moreText, FONT_BODY, moreX, messageY + messageH + 10, ANCHOR_MORE);
    }
    m_later.render(g, originY, originX);
    m_decline.render(g, originY, originX);
    m_accept.render(g, originY, originX);
}

bool PendingChallengeWindow::pointerReleased(int x, int y, int id) {
    if (m_later.contains(x, y)) {
        m_later.pointerReleased(x, y, id);
        close(RESULT_LATER);
        return true;
    }
    if (m_later.isPressed()) {
        m_later.unpress();
    }
    if (m_decline.contains(x, y)) {
        m_decline.pointerReleased(x, y, id);
        close(RESULT_CANCEL);
        return true;
    }
    if (m_decline.isPressed()) {
        m_decline.unpress();
    }
    if (m_accept.contains(x, y)) {
        m_accept.pointerReleased(x, y, id);
        close(RESULT_OK);
        return true;
    }
    if (m_accept.isPressed()) {
        m_accept.unpress();
    }
    return false;
}

PendingChallengeWindow::PendingChallengeWindow(const std::string& challengeId, const std::string& name)
    : m_challengeId(challengeId),
      m_frame(0x32, 0, m_width - 100, 0),
      m_accept(TEXT_ACCEPT),
      m_decline(TEXT_DECLINE),
      m_later(TEXT_LATER) {
    int wrapWidth = m_frame.getWidth() - 0x3c;
    m_name.wrapString(new midp::String(name.c_str()), FONT_BODY, wrapWidth);
    m_message.wrapString(TEXT_MESSAGE, FONT_BODY, wrapWidth);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int frameH = m_name.getWrappedTextHeight() + m_message.getWrappedTextHeight() +
                 text->getLineHeight(FONT_BODY) + 0x28;
    m_frame.setHeight(frameH);
    m_frame.setY((m_height - m_frame.getHeight()) >> 1);
    int laterW = m_later.getWidth();
    int laterH = m_later.getHeight();
    m_later.setX((m_width - 10) - laterW);
    m_later.setY((m_height - 10) - laterH);
    int declineW = m_decline.getWidth();
    m_decline.setX((m_later.getX() - 0x14) - declineW);
    m_decline.setY(m_later.getY());
    int acceptW = m_accept.getWidth();
    m_accept.setX((m_decline.getX() - 0x14) - acceptW);
    m_accept.setY(m_decline.getY());
}

PendingChallengeWindow::~PendingChallengeWindow() {}

}
