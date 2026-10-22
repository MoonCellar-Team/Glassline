#pragma once

#include "runtime/midp/Display.h"

namespace midp {

class DisplayIPhone : public Display {
public:
    DisplayIPhone(int width, int height, void* view);
    virtual ~DisplayIPhone();

    static DisplayIPhone* getStaticDisplay();
    virtual void repaint(int x, int y, int width, int height);
    virtual void serviceRepaints();
    void* getGL();
    virtual void refresh();
    virtual void hideNotify();
    virtual void showNotify();

    void* m_view;
    bool m_dirty;
};

}
