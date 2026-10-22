#pragma once

#include "runtime/midp/array.h"

class AnimPlayerQuad;
class QuadGroup;
class QuadImage;
class QuadMesh;
class QuadTexture;
namespace midp {
class Graphics;
class InputStream;
}
namespace m3g {
class Camera;
class CompositingMode;
class Graphics3D;
class IndexBuffer;
class Node;
class PolygonMode;
class VertexArray;
class World;
}

class QuadManager {
public:
    enum Anchor {
        ANCHOR_LEFT = 0x01,
        ANCHOR_HCENTER = 0x02,
        ANCHOR_RIGHT = 0x04,
        ANCHOR_TOP = 0x08,
        ANCHOR_VCENTER = 0x10,
        ANCHOR_BOTTOM = 0x20,
    };

    static const short QUAD_TEXTURE[];
    static const short QUAD_FILTER[];
    static const short QUAD_BLENDING[];

    QuadManager();
    ~QuadManager();

    AnimPlayerQuad* getAnimPlayer(int anim);
    bool isAnimating(int anim);
    m3g::World* getWorld();
    int getAnimCurrentFrameIndex(int anim);
    void render(midp::Graphics* g, int scope);
    void render(midp::Graphics* g);
    void setViewport(int x, int y, int width, int height, int rotation);
    void clearViewport();
    void loadQuads(int set);
    void freeQuads(int set);

    void freeTexture(int texture);
    void getMeshBounds(int mesh, int& x, int& y, int& width, int& height);
    bool isPointWithinMesh(int mesh, int x, int y, int marginX, int marginY);
    int getMeshBottom(int mesh);
    int getMeshTop(int mesh);
    int getMeshRight(int mesh);
    int getMeshHeight(int mesh);
    int getMeshWidth(int mesh);
    int getMeshY(int mesh);
    int getMeshX(int mesh);
    QuadMesh* getMesh(int mesh);
    bool getMeshVisible(int mesh);
    void setGroupPosition(int group, float x, float y);
    void setMeshVisible(int mesh, bool visible);
    void setGroupVisible(int group, bool visible);
    void setMeshBounds(int mesh, float x, float y, float width, float height, int flags);
    void setMeshAlpha(int mesh, float alpha);
    void setMeshRotation(int mesh, float degrees);
    void setMeshColor(int mesh, unsigned int argb);

    void setAnimTime(int anim, int time);
    void setAnimFrame(int anim, int frame);
    void playAnim(int anim, int mode);
    void playAnimReverse(int anim, int mode);
    void playAnimAcross(int anim, int mode, int from, int to);
    void playAnimTo(int anim, int frame);

    void render(m3g::Graphics3D* g3d, int scope);
    void render(m3g::Graphics3D* g3d);
    void updateAnim(int anim, int dt);
    void updateMesh(QuadMesh* mesh);
    bool isPointWithinMesh(int mesh, int x, int y);
    QuadTexture* getTexture(int texture);

    void setMeshPosition(int mesh, float x, float y, int anchor);

    void freeMesh(int mesh);
    void removeRawChild(m3g::Node* node);
    void addRawChild(m3g::Node* node);
    void ensureTextureLoaded(QuadTexture& texture);
    void loadMesh(int mesh);
    void loadGroup(QuadGroup& group);
    void loadQuadData(midp::InputStream* stream, int width, int height);

    void threadUnlock();
    void threadLock();

    bool m_locked;
    m3g::World* m_world;
    m3g::Camera* m_camera;
    midp::array<m3g::CompositingMode*> m_compositingModes;
    midp::array<QuadImage> m_images;
    midp::array<QuadTexture> m_textures;
    midp::array<QuadGroup> m_groups;
    midp::array<QuadMesh> m_meshes;
    midp::array<AnimPlayerQuad*> m_animPlayers;
    m3g::PolygonMode* m_polygonMode;
    m3g::VertexArray* m_positions;
    m3g::IndexBuffer* m_indices;
};
