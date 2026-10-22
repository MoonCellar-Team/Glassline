#pragma once

namespace m3g {
class Group;
}

class QuadGroup {
public:
    QuadGroup();

    m3g::Group* m_group;
    int m_parent;
    bool m_visible;
    int m_firstGroup;
    int m_numGroups;
    int m_firstMesh;
    int m_numMeshes;
    int m_x;
    int m_y;
};
