#pragma once
#include "runtime/midp/Object.h"
namespace midp { class Class; }
namespace m3g { class Node; class Transform; class Appearance; class IndexBuffer; class VertexBuffer; }

namespace m3g {

class RenderNode : public midp::Object {
public:
    RenderNode(const RenderNode& other);
    RenderNode(const Node* node,int submesh,Transform* transform,bool skinned);
    virtual ~RenderNode();
    virtual midp::Class* getClass() const;
    RenderNode& operator=(const RenderNode& other);
    bool operator<(const RenderNode& other) const;
    const Node* m_node;
    Transform* m_transform;
    int m_layer;
    int m_blending;
    bool m_skinned;
    Appearance* m_appearance;
    IndexBuffer* m_indices;
    VertexBuffer* m_vertices;
};

}
