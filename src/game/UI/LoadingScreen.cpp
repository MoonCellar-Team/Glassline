#include "game/LoadingScreen.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "game/UI/LoadingScreenImageElement.h"
#include "game/UI/LoadingScreenStringElement.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/ReferenceCountedPointer.h"

static const int ELEMENT_WIDTH = 0x100;
static const int ELEMENT_HEIGHT = 0x400;
static const int SHORT_CONTENT = 0x200;
static const int IMAGE_TALL = 0xec;
static const int IMAGE_SHORT = 0xed;
static const int ANCHOR_TOP_LEFT = 9;

static const int KIND_STRING = 0;
static const int KIND_IMAGE = 1;
static const int GAP_CENTERED = 10;
static const int GAP_PLAIN = 25;

namespace UI {

int LoadingScreen::getColour() const {
    return m_colour;
}

int LoadingScreen::getTotalHeight() {
    return m_totalHeight;
}

LoadingScreen::~LoadingScreen() {
    for (std::vector<WindowElement*>::iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
        delete *it;
    }
}

void LoadingScreen::render(midp::Graphics* g, int originY, int originX) {
    int imageId = IMAGE_TALL;
    if (m_totalHeight < SHORT_CONTENT) {
        m_height = SHORT_CONTENT;
        imageId = IMAGE_SHORT;
    }
    ResourceManager& resources = AppEngine::getCanvas()->getResourceManager();
    midp::ReferenceCountedPointer<midp::Image> image(resources.loadImage(imageId));
    g->drawRegion(image, 0, 0, m_width, m_height, 0, m_x, m_y, ANCHOR_TOP_LEFT);
    for (std::vector<WindowElement*>::iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
        (*it)->render(g, originY, originX);
    }
}

void LoadingScreen::loadScreen(midp::DataInputStream& in) {
    m_colour = in.readByte();
    int count = in.readInt();
    int total = 0;
    for (int i = 0; i < count; ++i) {
        int kind = in.readByte();
        if (kind == KIND_STRING) {
            LoadingScreenStringElement* text = new LoadingScreenStringElement(in, total);
            m_elements.push_back(text);
            total += text->getHeight();
            if (text->isCentered()) {
                total += GAP_CENTERED;
            } else if (!text->usesLargeFont()) {
                total += GAP_PLAIN;
            }
        } else if (kind == KIND_IMAGE) {
            LoadingScreenImageElement* image = new LoadingScreenImageElement(in, 0);
            m_elements.push_back(image);
            total += image->getHeight();
        }
    }
    m_totalHeight = total;
}

LoadingScreen::LoadingScreen(midp::DataInputStream& in)
    : WindowElement(0, 0, ELEMENT_WIDTH, ELEMENT_HEIGHT),
      m_totalHeight(0),
      m_colour(0) {
    loadScreen(in);
}

}
