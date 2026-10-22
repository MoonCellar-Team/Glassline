#include "game/M3GAssets_TexGroup.h"

#include "game/M3GAssets_Texture.h"

void M3GAssets_TexGroup::free() {
    if (!m_loaded || m_cacheFlags != 0 || m_modelRefs != 0) {
        return;
    }
    m_defaultTexture->freeInternalAssetReference();
    int count = m_textures.m_length;
    for (int i = 0; i < count; i++) {
        m_textures[i]->freeInternalAssetReference();
    }
    m_appGroup.clearAppearances();
    m_loaded = false;
}

void M3GAssets_TexGroup::freeForModel() {
    m_modelRefs--;
    free();
}

void M3GAssets_TexGroup::freeCached(int flags) {
    m_cacheFlags &= ~flags;
    free();
}

void M3GAssets_TexGroup::addElement(int id, M3GAssets_Texture* texture) {
    m_textures[m_appGroup.getNumMappings()] = texture;
    m_appGroup.addElement(id, NULL);
}

M3GAssets_TexGroup::~M3GAssets_TexGroup() {
    m_cacheFlags = 0;
    m_modelRefs = 0;
    free();
}

M3GAssets_TexGroup::M3GAssets_TexGroup(int capacity, M3GAssets_Texture* defaultTexture)
    : m_appGroup(capacity, NULL),
      m_textures(capacity),
      m_defaultTexture(defaultTexture),
      m_loaded(false),
      m_cacheFlags(0),
      m_modelRefs(0) {}

const M3GAssets_AppGroup& M3GAssets_TexGroup::load() {
    if (!m_loaded) {
        m_appGroup.setDefaultAppearance(m_defaultTexture->loadForInternalAsset());
        int count = m_textures.m_length;
        for (int i = 0; i < count; i++) {
            m_appGroup.setMappedAppearance(m_textures[i]->loadForInternalAsset(), i);
        }
        m_loaded = true;
    }
    return m_appGroup;
}

const M3GAssets_AppGroup& M3GAssets_TexGroup::loadForModel() {
    m_modelRefs++;
    return load();
}

const M3GAssets_AppGroup& M3GAssets_TexGroup::loadCached(int flags) {
    m_cacheFlags |= flags;
    return load();
}
