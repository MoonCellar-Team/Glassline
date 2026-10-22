#include "runtime/midp/DisplayIPhone.h"

#include "runtime/midp/GLES11.h"
#include "runtime/midp/GLES20.h"
#include "runtime/midp/Graphics.h"
#include "sdk/UIKit.h"

namespace {

class HostView {
public:
    int getGLESMajorVersion();
    int getGLESMinorVersion();
    void* getGL();
    void draw();
};

}

namespace midp {

static DisplayIPhone* s_display;

DisplayIPhone* DisplayIPhone::getStaticDisplay() {
    return s_display;
}

void DisplayIPhone::repaint(int, int, int, int) {
    m_dirty = true;
}

void DisplayIPhone::serviceRepaints() {
    if (!m_dirty)
        return;
    static_cast<HostView*>(m_view)->draw();
}

void* DisplayIPhone::getGL() {
    return static_cast<HostView*>(m_view)->getGL();
}

void DisplayIPhone::refresh() {
    Display::refresh();
    m_dirty = false;
}

void DisplayIPhone::hideNotify() {
    Display::hideNotify();
}

void DisplayIPhone::showNotify() {
    int deviceOrientation = UIDevice::currentDevice()->orientation();
    int orientation = 2;
    if (deviceOrientation == 4) {
        UIApplication::sharedApplication()->setStatusBarOrientation(4);
        orientation = 3;
    } else if (deviceOrientation == 3) {
        UIApplication::sharedApplication()->setStatusBarOrientation(3);
    }
    setOrientation(orientation);
    Display::showNotify();
}

DisplayIPhone::~DisplayIPhone() {}

DisplayIPhone::DisplayIPhone(int width, int height, void* view)
    : Display(width, height), m_view(view), m_dirty(false) {
    HostView* host = static_cast<HostView*>(view);
    int major = host->getGLESMajorVersion();
    int minor = host->getGLESMinorVersion();
    if (major == 2 && minor == 0) {
        setGraphics(new GraphicsIPhoneGLES20(this, static_cast<GLES20*>(host->getGL())));
    } else if (major == 1 && minor == 1) {
        setGraphics(new GraphicsIPhoneGLES11(this, static_cast<GLES11*>(host->getGL())));
    }
    s_display = this;
}

}
