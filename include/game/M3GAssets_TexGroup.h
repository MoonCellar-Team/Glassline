#pragma once

#include "game/M3GAssets_AppGroup.h"
#include "runtime/midp/array.h"

class M3GAssets_Texture;

class M3GAssets_TexGroup {
public:
    M3GAssets_TexGroup(int capacity, M3GAssets_Texture* defaultTexture);
    ~M3GAssets_TexGroup();

    void addElement(int id, M3GAssets_Texture* texture);
    const M3GAssets_AppGroup& load();
    const M3GAssets_AppGroup& loadCached(int flags);
    const M3GAssets_AppGroup& loadForModel();
    void free();
    void freeCached(int flags);
    void freeForModel();

    M3GAssets_AppGroup m_appGroup;
    midp::array<M3GAssets_Texture*> m_textures;
    M3GAssets_Texture* m_defaultTexture;
    bool m_loaded;
    int m_cacheFlags;
    int m_modelRefs;
};
