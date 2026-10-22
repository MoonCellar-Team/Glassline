#include "game/MonkeyApp.h"

#include <stddef.h>

#include "ea_online/ea/EASpywareManager.h"
#include "game/AppEngine.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/String.h"
#include "runtime/midp/TextBox.h"
#include "runtime/midp/Thread.h"

static const int SPYWARE_EVENT_APP_EXIT = 20000;

midp::String* MonkeyApp::getInputString(midp::String* title, int maxSize) {
    m_inputActive = true;
    midp::TextBox* box = new midp::TextBox(title, NULL, maxSize, 0, 0);
    midp::INCREF(box);
    midp::Displayable* prev = m_display->getCurrent();
    midp::INCREF(prev);
    box->show(m_display);
    while (!m_destroying && box->getString() == NULL) {
        midp::Thread::yield();
    }
    m_display->setCurrent(m_destroying ? NULL : prev);
    midp::DECREF(prev);
    midp::String* text = box->getString();
    midp::INCREF(text);
    midp::DECREF(box);
    midp::DECREF_NO_DELETE(text);
    m_inputActive = false;
    return text;
}

void MonkeyApp::receivedNotification(std::map<std::string, std::string> info) {
    if (m_engine != NULL) {
        m_engine->receivedNotification(info);
    }
}

void MonkeyApp::close() {
    if (m_engine != NULL) {
        m_engine->end();
    }
    midp::Display* display = m_display;
    if (display == NULL) {
        return;
    }
    m_display = NULL;
    display->setCurrent(NULL);
    midp::DECREF(display);
}

void MonkeyApp::startApp() {
    midp::Display* display = midp::Display::getDisplay(this);
    midp::INCREF(display);
    midp::DECREF(m_display);
    m_display = display;
    m_engine->setFullScreenMode(true);
    m_engine->setFullScreenMode(true);
    bool first = !m_started;
    if (first) {
        m_engine->start();
        m_started = true;
        m_engine->startThread();
    }
    if (m_engine->isPaused()) {
        m_engine->resumeGame();
        m_engine->startThread();
    }
    if (first) {
        m_display->setCurrent(m_engine);
    }
}

void MonkeyApp::pauseApp() {
    m_engine->pauseGame();
}

void MonkeyApp::destroyApp(bool) {
    m_destroying = true;
    while (m_inputActive) {
        midp::Thread::yield();
    }
    ea::EASpywareManager::getInstance()->logEvent(SPYWARE_EVENT_APP_EXIT);
    close();
    notifyDestroyed();
}

MonkeyApp::~MonkeyApp() {
    midp::DECREF(m_engine);
}

MonkeyApp::MonkeyApp()
    : m_started(false), m_engine(NULL), m_display(NULL), m_inputActive(false), m_destroying(false) {
    AppEngine::createAppEngine(this);
    m_engine = AppEngine::getCanvas();
    midp::INCREF(m_engine);
}
