#pragma once

namespace m3g {
class Node;
}
class M3GAssets_File;

class M3GAssets_Node {
public:
    M3GAssets_Node(M3GAssets_File* file, int userId, bool unique);
    ~M3GAssets_Node();

    void load();
    m3g::Node* loadUnique();
    m3g::Node* loadUniqueCached(int flags);
    m3g::Node* loadForModel();
    void addModelRef();
    void free();
    void freeCached(int flags);
    void freedFromModel();

    int m_cacheFlags;
    M3GAssets_File* m_file;
    int m_userId;
    m3g::Node* m_node;
    bool m_unique;
    int m_modelLoads;
    int m_modelRefs;
};
