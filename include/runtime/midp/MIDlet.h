#pragma once

#include <map>
#include <string>

#include "runtime/midp/Object.h"

namespace midp {

class Class;
class Display;

class MIDlet : public Object {
public:
    MIDlet();
    virtual ~MIDlet();
    virtual Class* getClass() const;
    virtual void destroyApp(bool unconditional) = 0;
    virtual void pauseApp() = 0;
    virtual void startApp() = 0;
    virtual void receivedNotification(std::map<std::string, std::string> info) = 0;
    Display* getDisplay();
    void setDisplay(Display* display);
    void notifyDestroyed();

    Display* m_display;
};

}
