#include "runtime/midp/MIDlet.h"

#include <stddef.h>

#include "runtime/midp/Display.h"
#include "runtime/midp/MIDletClass.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/Runtime.h"

namespace midp {

Class* MIDlet::getClass() const {
    return new MIDletClass();
}

Display* MIDlet::getDisplay() {
    return m_display;
}

void MIDlet::setDisplay(Display* display) {
    INCREF(display);
    DECREF(m_display);
    m_display = display;
}

void MIDlet::notifyDestroyed() {
    Runtime::getRuntime()->destroyMIDlet(this);
}

MIDlet::~MIDlet() {}

MIDlet::MIDlet() : m_display(NULL) {}

}
