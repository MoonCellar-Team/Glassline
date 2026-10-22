#pragma once

#include "game/UI/WindowElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Graphics;
class String;
}

namespace UI {

class LanguageItem : public WindowElement {
public:
    LanguageItem(int language, midp::ReferenceCountedPointer<midp::String> name);
    virtual ~LanguageItem();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int getLangId() const;

    int m_langId;
    midp::ReferenceCountedPointer<midp::String> m_name;
};

}
