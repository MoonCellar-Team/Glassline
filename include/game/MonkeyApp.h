#pragma once

#include <map>
#include <string>

#include "runtime/midp/MIDlet.h"

class AppEngine;
namespace midp {
class Display;
class String;
}

class MonkeyApp : public midp::MIDlet {
public:
    MonkeyApp();
    virtual ~MonkeyApp();
    virtual void destroyApp(bool unconditional);
    virtual void pauseApp();
    virtual void startApp();
    virtual void receivedNotification(std::map<std::string, std::string> info);

    midp::String* getInputString(midp::String* title, int maxSize);
    void close();

    bool m_started;
    AppEngine* m_engine;
    midp::Display* m_display;
    bool m_inputActive;
    bool m_destroying;
};
