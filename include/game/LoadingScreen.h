#pragma once

#include <vector>

#include "game/UI/WindowElement.h"

namespace midp {
class DataInputStream;
}

namespace UI {

class LoadingScreen : public WindowElement {
public:
    explicit LoadingScreen(midp::DataInputStream& in);
    virtual ~LoadingScreen();
    virtual void render(midp::Graphics* g, int originY, int originX);

    void loadScreen(midp::DataInputStream& in);
    int getTotalHeight();
    int getColour() const;

    int m_totalHeight;
    int m_colour;
    std::vector<WindowElement*> m_elements;
};

}
