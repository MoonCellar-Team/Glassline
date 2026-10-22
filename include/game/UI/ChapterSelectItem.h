#pragma once

#include "game/UI/WindowElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class Level;

namespace midp {
class Graphics;
class String;
}

namespace UI {

class ChapterSelectItem : public WindowElement {
public:
    explicit ChapterSelectItem(const Level& level);
    virtual ~ChapterSelectItem();

    virtual void render(midp::Graphics* g, int originY, int originX);

    const Level* getLevelObject() const;

    const Level* m_level;
    midp::ReferenceCountedPointer<midp::String> m_name;
};

}
