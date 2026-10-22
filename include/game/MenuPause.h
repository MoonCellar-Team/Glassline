#pragma once

#include "game/Menu.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

class MenuPause : public Menu {
public:
    enum Item {
        ITEM_HELP = 0x803,
        ITEM_OPTIONS = 0x81e,
        ITEM_SETTINGS = 0x822,
        ITEM_MEDIA_LIBRARY = 0x826,
        ITEM_RESUME = 0x829,
        ITEM_RESTART = 0x82a,
        ITEM_QUIT = 0x82b,
        ITEM_SOUND = 0x8d7,
    };

    MenuPause();
    virtual ~MenuPause();
    virtual void render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height);
    virtual void render2D(midp::Graphics* g, int width, int height);

    void createSingle();
    void createSplit(bool first, int height);

    midp::ReferenceCountedPointer<midp::String> m_levelName;
    midp::ReferenceCountedPointer<midp::String> m_title;
    int m_quadGroup;
};
