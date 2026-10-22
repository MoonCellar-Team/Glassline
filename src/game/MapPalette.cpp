#include "game/MapPalette.h"

#include "game/AppEngine.h"
#include "game/M3GAssets.h"
#include "game/ModelSet.h"
#include "game/ResourceManager.h"
#include "m3g/Node.h"

static const int TEXTURE_CACHE_MAP = 8;

MapPalette::~MapPalette() {}

m3g::Node* MapPalette::getNode(int userId) {
    return static_cast<m3g::Node*>(m_root->find(userId));
}

m3g::Node* MapPalette::createUniqueNode(int userId) {
    m3g::Object3D* node = m_root->find(userId);
    if (node == NULL) {
        return NULL;
    }
    return static_cast<m3g::Node*>(node->duplicate());
}

MapPalette::MapPalette(int resId, const ModelSet* modelSet) {
    ResourceManager& resources = AppEngine::getCanvas()->getResourceManager();
    M3GAssets* assets = AppEngine::getM3GAssets();
    m_root = static_cast<m3g::Node*>(resources.loadM3GNode(resId));
    midp::DECREF(m_root);
    const M3GAssets_AppGroup& textures = assets->loadTextureGroup(modelSet->getModelId(0), TEXTURE_CACHE_MAP);
    M3GAssets::applyAppearanceGroup(m_root, textures);
    M3GAssets::commit(m_root);
}
