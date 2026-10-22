#include "game/M3GAssets_Image.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "m3g/GLHelpers.h"

M3GAssets_Image::M3GAssets_Image(int resId) : m_resId(resId), m_image(NULL), m_useCount(0) {}

M3GAssets_Image::~M3GAssets_Image() {
    midp::DECREF(m_image);
    m_image = NULL;
}

void M3GAssets_Image::freeImage() {
    if (--m_useCount == 0) {
        midp::DECREF(m_image);
        m_image = NULL;
    }
}

m3g::Image2D* M3GAssets_Image::loadImage() {
    m_useCount++;
    if (m_image == NULL) {
        m_image = AppEngine::getCanvas()->getResourceManager().loadM3GImage2D(m_resId);
    }
    return m_image;
}
