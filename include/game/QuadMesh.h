#pragma once

class QuadTexture;
namespace m3g {
class Mesh;
class VertexBuffer;
}

class QuadMesh {
public:
    enum Attrib {
        ATTRIB_X = 0,
        ATTRIB_Y = 1,
        ATTRIB_WIDTH = 2,
        ATTRIB_HEIGHT = 3,
        ATTRIB_ALPHA = 4,
        ATTRIB_ROTATION = 5,
        NUM_ATTRIBS = 6,
    };

    QuadMesh();
    void setVisible(bool visible);

    m3g::Mesh* m_node;
    m3g::VertexBuffer* m_vertexBuffer;
    int m_group;
    int m_texture;
    QuadTexture* m_textureEntry;
    int m_scope;
    int m_layer;
    int m_color;
    float m_x;
    float m_y;
    int m_attribs[6];
    bool m_visible;
    bool m_dirty;
};
