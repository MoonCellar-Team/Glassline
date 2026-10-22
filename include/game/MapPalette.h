#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"

class ModelSet;
namespace m3g {
class Node;
}

class MapPalette {
public:
    MapPalette(int resId, const ModelSet* modelSet);
    ~MapPalette();

    m3g::Node* getNode(int userId);
    m3g::Node* createUniqueNode(int userId);

    midp::ReferenceCountedPointer<m3g::Node> m_root;
};
