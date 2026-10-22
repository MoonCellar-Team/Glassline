#include "m3g/RenderNode.h"
#include "m3g/GLHelpers.h"
#include "m3g/Appearance.h"
#include "m3g/CompositingMode.h"

namespace m3g {

RenderNode::RenderNode(const RenderNode& other) : m_node(other.m_node),m_transform(other.m_transform),m_layer(other.m_layer),m_blending(other.m_blending),m_skinned(other.m_skinned),m_appearance(other.m_appearance),m_indices(other.m_indices),m_vertices(other.m_vertices) {}

RenderNode::~RenderNode() { m_node=nullptr; m_transform=nullptr; }

midp::Class* RenderNode::getClass() const { return nullptr; }

RenderNode& RenderNode::operator=(const RenderNode& other) {
    m_node=other.m_node; m_transform=other.m_transform; m_layer=other.m_layer;
    m_appearance=other.m_appearance; m_indices=other.m_indices; m_vertices=other.m_vertices;
    m_blending=other.m_blending; m_skinned=other.m_skinned; return *this;
}

RenderNode::RenderNode(const Node* node,int submesh,Transform* transform,bool skinned) :
    m_node(nullptr),m_transform(nullptr),m_layer(0),m_blending(0x44),m_skinned(skinned),m_appearance(nullptr),m_indices(nullptr),m_vertices(nullptr) {
    if(!node) return;
    const Mesh* mesh=m3g_cast<Mesh>(node); if(!mesh) return;
    Appearance* appearance=mesh->getAppearance(submesh); IndexBuffer* indices=mesh->getIndexBuffer(submesh);
    if(!appearance || !indices) return;
    m_node=node; m_transform=transform; m_layer=appearance->getLayer(); m_appearance=appearance; m_indices=indices; m_vertices=mesh->getVertexBuffer();
    CompositingMode* compositing=appearance->getCompositingMode(); if(compositing) m_blending=compositing->getBlending();
}

bool RenderNode::operator<(const RenderNode& other) const {
    if(m_layer!=other.m_layer) return m_layer<other.m_layer;
    auto address=[](const void* pointer) { return reinterpret_cast<__UINTPTR_TYPE__>(pointer); };
    if(m_skinned && other.m_skinned && m_vertices!=other.m_vertices) return address(m_vertices)<address(other.m_vertices);
    if(m_blending!=other.m_blending && (m_blending==0x44 || other.m_blending==0x44)) return m_blending==0x44;
    if(m_appearance!=other.m_appearance) {
        for(int i=0;i<2;++i) {
            Texture2D* left=m_appearance->getTexture(i); Texture2D* right=other.m_appearance->getTexture(i);
            if(left!=right) return address(left)<address(right);
        }
        Fog* leftFog=m_appearance->getFog(); Fog* rightFog=other.m_appearance->getFog();
        if(leftFog!=rightFog) return address(leftFog)<address(rightFog);
        CompositingMode* leftComp=m_appearance->getCompositingMode(); CompositingMode* rightComp=other.m_appearance->getCompositingMode();
        if(leftComp!=rightComp) return address(leftComp)<address(rightComp);
        PolygonMode* leftPoly=m_appearance->getPolygonMode(); PolygonMode* rightPoly=other.m_appearance->getPolygonMode();
        if(leftPoly!=rightPoly) return address(leftPoly)<address(rightPoly);
    }
    if(m_vertices!=other.m_vertices) return address(m_vertices)<address(other.m_vertices);
    return address(m_indices)<address(other.m_indices);
}

}
