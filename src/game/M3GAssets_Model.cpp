#include "game/M3GAssets_Model.h"

#include "game/M3GAssets.h"
#include "game/M3GAssets_Node.h"
#include "game/M3GAssets_TexGroup.h"
#include "game/M3GAssets_Texture.h"
#include "m3g/Node.h"

M3GAssets_Model::M3GAssets_Model(M3GAssets_Node* node, M3GAssets_Texture* texture,
                                 M3GAssets_TexGroup* texGroup, bool unique, bool commit)
    : m_cacheFlags(0),
      m_node(node),
      m_texture(texture),
      m_texGroup(texGroup),
      m_model(NULL),
      m_unique(unique),
      m_commit(commit) {
    if (m_node != NULL) {
        m_node->addModelRef();
    }
}

void M3GAssets_Model::freeCached(int flags) {
    m_cacheFlags &= ~flags;
    if (m_model == NULL || m_cacheFlags != 0) {
        return;
    }
    midp::DECREF(m_model);
    m_model = NULL;
    m_node->freedFromModel();
    if (m_texture != NULL) {
        m_texture->freeInternalAssetReference();
    }
    if (m_texGroup != NULL) {
        m_texGroup->freeForModel();
    }
}

M3GAssets_Model::~M3GAssets_Model() {
    freeCached(m_cacheFlags);
}

m3g::Node* M3GAssets_Model::loadCached(int flags) {
    m_cacheFlags |= flags;
    if (m_model == NULL) {
        m_model = m_node->loadForModel();
        midp::INCREF(m_model);
        if (m_texture != NULL) {
            M3GAssets::applyAppearance(m_model, m_texture->loadForInternalAsset());
        } else if (m_texGroup != NULL) {
            M3GAssets::applyAppearanceGroup(m_model, m_texGroup->loadForModel());
        }
        if (m_commit) {
            M3GAssets::commit(m_model);
        }
    }
    if (m_unique) {
        m3g::Node* copy = static_cast<m3g::Node*>(m_model->duplicate());
        M3GAssets::cacheSkinTransforms(copy);
        return copy;
    }
    M3GAssets::cacheSkinTransforms(m_model);
    return m_model;
}
