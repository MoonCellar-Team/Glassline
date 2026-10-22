#include "game/M3GAssets_Texture.h"

#include "game/M3GAssets.h"
#include "game/M3GAssets_Image.h"
#include "m3g/Appearance.h"

M3GAssets_Texture::M3GAssets_Texture(m3g::Appearance* base, M3GAssets_Image* image,
                                     int levelFilter, int imageFilter, int blending)
    : m_base(base),
      m_image(image),
      m_appearance(NULL),
      m_levelFilter(levelFilter),
      m_imageFilter(imageFilter),
      m_blending(blending),
      m_cacheFlags(0),
      m_internalRefs(0) {
    midp::INCREF(base);
}

void M3GAssets_Texture::free() {
    if (m_cacheFlags != 0 || m_internalRefs != 0) {
        return;
    }
    if (m_appearance == NULL) {
        return;
    }
    m_image->freeImage();
    midp::DECREF(m_appearance);
    m_appearance = NULL;
}

void M3GAssets_Texture::freeInternalAssetReference() {
    m_internalRefs--;
    free();
}

void M3GAssets_Texture::freeCached(int flags) {
    m_cacheFlags &= ~flags;
    free();
}

M3GAssets_Texture::~M3GAssets_Texture() {
    m_cacheFlags = 0;
    m_internalRefs = 0;
    free();
    midp::DECREF(m_base);
    m_base = NULL;
}

m3g::Appearance* M3GAssets_Texture::load() {
    if (m_appearance == NULL) {
        m_appearance = M3GAssets::createTexturedAppearance(m_base, m_image->loadImage(), m_levelFilter,
                                                           m_imageFilter, m_blending);
        midp::INCREF(m_appearance);
    }
    return m_appearance;
}

m3g::Appearance* M3GAssets_Texture::loadForInternalAsset() {
    m_internalRefs++;
    return load();
}

m3g::Appearance* M3GAssets_Texture::loadCached(int flags) {
    m_cacheFlags |= flags;
    return load();
}

m3g::Appearance* M3GAssets_Texture::get() {
    return m_appearance;
}
