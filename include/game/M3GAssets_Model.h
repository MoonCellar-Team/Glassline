#pragma once

namespace m3g {
class Node;
}
class M3GAssets_Node;
class M3GAssets_Texture;
class M3GAssets_TexGroup;

class M3GAssets_Model {
public:
    M3GAssets_Model(M3GAssets_Node* node, M3GAssets_Texture* texture, M3GAssets_TexGroup* texGroup,
                    bool unique, bool commit);
    ~M3GAssets_Model();

    m3g::Node* loadCached(int flags);
    void freeCached(int flags);

    int m_cacheFlags;
    M3GAssets_Node* m_node;
    M3GAssets_Texture* m_texture;
    M3GAssets_TexGroup* m_texGroup;
    m3g::Node* m_model;
    bool m_unique;
    bool m_commit;
};
