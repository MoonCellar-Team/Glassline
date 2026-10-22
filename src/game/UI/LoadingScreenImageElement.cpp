#include "game/UI/LoadingScreenImageElement.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/ReferenceCountedPointer.h"

static const int ANCHOR_HCENTER = 2;
static const int ANCHOR_RIGHT = 4;
static const int ANCHOR_VCENTER = 0x10;
static const int ANCHOR_BOTTOM = 0x20;
static const int DRAW_Y_BIAS = 18;

static void applyAnchor(int& x, int& y, int width, int height, int anchor) {
    if ((anchor & ANCHOR_VCENTER) != 0) {
        y += height >> 1;
    } else if ((anchor & ANCHOR_BOTTOM) != 0) {
        y += height;
    }
    if ((anchor & ANCHOR_HCENTER) != 0) {
        x += width >> 1;
    } else if ((anchor & ANCHOR_RIGHT) != 0) {
        x += width;
    }
}

namespace UI {

void LoadingScreenImageElement::render(midp::Graphics* g, int originY, int originX) {
    int x = m_x;
    int y = m_y;
    applyAnchor(x, y, m_width, m_height, m_anchor);
    ResourceManager& resources = AppEngine::getCanvas()->getResourceManager();
    midp::ReferenceCountedPointer<midp::Image> image(resources.loadImage(m_imageId));
    g->drawRegion(image, 0, 0, image->getWidth(), image->getHeight(), 0, x,
                  (y + DRAW_Y_BIAS) - m_height, m_anchor);
}

LoadingScreenImageElement::LoadingScreenImageElement(midp::DataInputStream& in, int)
    : WindowElement(0, 0, 0, 0),
      m_imageId(0),
      m_anchor(0) {
    in.readShort();
    int index = in.readShort();
    m_imageId = ResourceManager::LOADING_IMAGE_LOOKUP[index];
    in.readShort();
    m_anchor = in.readShort();
    m_x = in.readInt();
    m_y = in.readInt();
    m_width = in.readInt();
    m_height = in.readInt();
}

LoadingScreenImageElement::~LoadingScreenImageElement() {}

}
