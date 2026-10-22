#pragma once
namespace m3g { class Object3D; }

namespace m3g {

class Object3DFinder {
public:
    explicit Object3DFinder(int userID);
    Object3D* getFound() const;
    void find(Object3D* object);
    int m_userID;
    Object3D* m_found;
};

}
