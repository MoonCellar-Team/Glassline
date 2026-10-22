#pragma once

namespace m3g {
class Object3D;
}

class M3GAssets_File {
public:
    explicit M3GAssets_File(int resId);
    ~M3GAssets_File();

    m3g::Object3D* load();
    void free();

    int m_resId;
    m3g::Object3D* m_node;
    int m_useCount;
};
