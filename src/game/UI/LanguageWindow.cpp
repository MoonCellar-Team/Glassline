#include "game/UI/LanguageWindow.h"

#include "ea_online/ea/EASpywareManager.h"
#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "game/UI/LanguageItem.h"

static const int TEXT_TITLE = 0x903;
static const int TEXT_SUBTITLE = 0x81e;

static const int BACK_X = 0x6e;
static const int BACK_Y = 0x122;
static const int BACK_W = 0x324;
static const int BACK_H = 0xb4;
static const int ARROW_INSET = 0x24;

namespace UI {

bool LanguageWindow::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
    if (m_languages.contains(x, y)) {
        m_languages.pointerPressed(m_languages.toRelativeX(x), y, id);
    }
    return true;
}

bool LanguageWindow::pointerDragged(int x, int y, int id) {
    TitledWindow::pointerDragged(x, y, id);
    if (m_languages.contains(x, y)) {
        m_languages.pointerDragged(m_languages.toRelativeX(x), y, id);
    } else if (m_languages.isDragging()) {
        m_languages.pointerReleased(x, y, id);
    }
    return true;
}

void LanguageWindow::update(int dt) {
    TitledWindow::update(dt);
    m_languages.update(dt);
    LanguageItem* item = static_cast<LanguageItem*>(m_languages.getSelectedItem());
    m_language = item->getLangId();
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    if (m_closed || m_language != text->getCurrentLanguage()) {
        text->setCurrentLanguage(m_language);
        setTitles(TEXT_TITLE, TEXT_SUBTITLE);
    }
    if (!m_closed) {
        return;
    }
#ifdef MEDECOMP_PORT
    medecomp_language_changed(text->getCurrentLocale()->getCFString());
#endif
    ea::EASpywareManager* spyware = ea::EASpywareManager::getInstance();
    spyware->logLanguageChange(text->getCurrentLocale());
    spyware->setLanguage(text->getCurrentLocale());
    spyware->retryTickers();
}

void LanguageWindow::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    m_languages.render(g, originY, originX);
    m_right.render(g, originY, originX);
    m_left.render(g, originY, originX);
}

bool LanguageWindow::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    if (m_languages.contains(x, y)) {
        m_languages.pointerReleased(x, y, id);
    }
    if (m_right.contains(x, y)) {
        m_languages.next();
        m_right.pointerReleased(x, y, id);
    } else if (m_left.contains(x, y)) {
        m_languages.prev();
        m_left.pointerReleased(x, y, id);
    }
    return true;
}

LanguageWindow::LanguageWindow()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_language(0) {
    m_background.setPosition(BACK_X, BACK_Y);
    m_background.setDimensions(BACK_W, BACK_H);
    m_left.setPosition(m_background.getX() + ARROW_INSET,
                       m_background.getY() + ((m_background.getHeight() - m_left.getHeight()) >> 1));
    m_right.setPosition(m_background.getX() + m_background.getWidth() - ARROW_INSET - m_left.getWidth(),
                        m_background.getY() + ((m_background.getHeight() - m_right.getHeight()) >> 1));
    m_languages.setPosition((m_width - m_languages.getWidth()) >> 1,
                            m_background.getY() + ((m_background.getHeight() - m_languages.getHeight()) >> 1));
    m_language = AppEngine::getCanvas()->getTextManager()->getCurrentLanguage();
}

LanguageWindow::~LanguageWindow() {}

}
