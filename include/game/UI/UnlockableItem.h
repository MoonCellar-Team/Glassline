#pragma once

#include "game/UI/WindowElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Graphics;
class String;
}

namespace UI {

class UnlockableItem : public WindowElement {
public:
    UnlockableItem(int quadId, int imageId, int textId, int badges);
    virtual ~UnlockableItem();

    virtual void render(midp::Graphics* g, int originY, int originX);

    void saveToLibrary() const;
    bool isUnlocked() const;
    midp::ReferenceCountedPointer<midp::String> getDescription() const;
    int getQuadId() const;
    int getThumbWidth() const;
    int getThumbHeight() const;
    int getThumbX() const;
    int getThumbY() const;

    int m_quadId;
    int m_imageId;
    midp::ReferenceCountedPointer<midp::String> m_unlocked;
    midp::ReferenceCountedPointer<midp::String> m_locked;
    int m_badges;
    int m_thumbWidth;
    int m_thumbHeight;
    int m_thumbX;
    int m_thumbY;
};

}
