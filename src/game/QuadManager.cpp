#include "game/QuadManager.h"

#include <stddef.h>
#include <stdlib.h>

#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/M3GAssets.h"
#include "game/QuadGroup.h"
#include "game/QuadImage.h"
#include "game/QuadMesh.h"
#include "game/QuadTexture.h"
#include "game/ResourceManager.h"
#include "m3g/Appearance.h"
#include "m3g/Background.h"
#include "m3g/Camera.h"
#include "m3g/CompositingMode.h"
#include "m3g/Graphics3D.h"
#include "m3g/Image2D.h"
#include "m3g/Mesh.h"
#include "m3g/PolygonMode.h"
#include "m3g/Texture2D.h"
#include "m3g/TriangleStripArray.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/World.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/Thread.h"

static const float FIXED_ONE = 65536.0f;
static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;
static const float BYTE_TO_UNIT = 1.0f / 255.0f;

static const int SCOPE_DEFAULT = 1;

static const float CAMERA_NEAR = -10.0f;
static const float CAMERA_FAR = 10.0f;
static const float DEGREES_PER_ROTATION = 90.0f;

static const int TEXTURE_FILTER_NEAREST = 210;
static const int POLYGON_CULL_NONE = 162;
static const int VERTEX_TYPE_SHORT = 2;

static const int COMPOSITING_UNTEXTURED = 5;

static const int QUAD_VERTICES = 4;
static const float QUAD_POSITION_SCALE = 0.5f;

static const unsigned short NO_TEXTURE = 0xffff;

const short QuadManager::QUAD_TEXTURE[] = {
     74, 141, 140, 143, 145, 137,  43,  44,  45,  46,  47,  48,  49,
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
     63,  64,  65,  66,  67,  68,  69,  70, 120, 121, 122, 123, 118,
    138, 139, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
    167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 124, 150, 151,
    152, 153, 142, 119, 154, 155, 135, 136, 133, 134,  42, 117, 116,
};

const short QuadManager::QUAD_FILTER[] = {209, 0};

const short QuadManager::QUAD_BLENDING[] = {64, 65};

AnimPlayerQuad* QuadManager::getAnimPlayer(int anim) {
    if (anim == -1) {
        return NULL;
    }
    return m_animPlayers[anim];
}

bool QuadManager::isAnimating(int anim) {
    if (m_animPlayers == NULL) {
        return false;
    }
    return m_animPlayers[anim]->isAnimating();
}

m3g::World* QuadManager::getWorld() {
    return m_world;
}

int QuadManager::getAnimCurrentFrameIndex(int anim) {
    return m_animPlayers[anim]->getCurrentKeyframe();
}

void QuadManager::freeTexture(int texture) {
    QuadTexture& quad = m_textures[texture];
    if (--quad.m_refCount != 0) {
        return;
    }
    midp::DECREF(quad.m_texture);
    quad.m_texture = NULL;
    midp::DECREF(quad.m_texCoords);
    quad.m_texCoords = NULL;
    QuadImage& image = m_images[quad.m_image];
    if (--image.m_refCount != 0) {
        return;
    }
    midp::DECREF(image.m_image);
    image.m_image = NULL;
}

void QuadManager::getMeshBounds(int mesh, int& x, int& y, int& width, int& height) {
    QuadMesh& quad = m_meshes[mesh];
    x = (int)(quad.m_x + (float)((quad.m_attribs[QuadMesh::ATTRIB_X] - (quad.m_attribs[QuadMesh::ATTRIB_WIDTH] >> 1)) >> 16));
    y = (int)(quad.m_y + (float)((quad.m_attribs[QuadMesh::ATTRIB_Y] - (quad.m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 1)) >> 16));
    width = quad.m_attribs[QuadMesh::ATTRIB_WIDTH] >> 16;
    height = quad.m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 16;
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        x += group.m_x;
        y -= group.m_y;
        i = group.m_parent;
    }
}

bool QuadManager::isPointWithinMesh(int mesh, int x, int y, int marginX, int marginY) {
    QuadMesh& quad = m_meshes[mesh];
    if (quad.m_node == NULL || !quad.m_node->isRenderingEnabled()) {
        return false;
    }
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    getMeshBounds(mesh, left, top, width, height);
    left -= marginX;
    top -= marginY;
    width += marginX * 2;
    height += marginY * 2;
    return x >= left && x <= left + width && y >= top && y <= top + height;
}

int QuadManager::getMeshBottom(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    int bottom = (int)(quad.m_y + (float)((quad.m_attribs[QuadMesh::ATTRIB_Y] + (quad.m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 1)) >> 16));
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        i = group.m_parent;
        bottom -= group.m_y;
    }
    return bottom;
}

int QuadManager::getMeshTop(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    int top = (int)(quad.m_y + (float)((quad.m_attribs[QuadMesh::ATTRIB_Y] - (quad.m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 1)) >> 16));
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        i = group.m_parent;
        top -= group.m_y;
    }
    return top;
}

int QuadManager::getMeshRight(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    int right = (int)(quad.m_x + (float)((quad.m_attribs[QuadMesh::ATTRIB_X] + (quad.m_attribs[QuadMesh::ATTRIB_WIDTH] >> 1)) >> 16));
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        i = group.m_parent;
        right += group.m_x;
    }
    return right;
}

int QuadManager::getMeshHeight(int mesh) {
    return m_meshes[mesh].m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 16;
}

int QuadManager::getMeshWidth(int mesh) {
    return m_meshes[mesh].m_attribs[QuadMesh::ATTRIB_WIDTH] >> 16;
}

int QuadManager::getMeshY(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    int y = (int)quad.m_y + (quad.m_attribs[QuadMesh::ATTRIB_Y] >> 16);
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        i = group.m_parent;
        y -= group.m_y;
    }
    return y;
}

int QuadManager::getMeshX(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    int x = (int)quad.m_x + (quad.m_attribs[QuadMesh::ATTRIB_X] >> 16);
    for (int i = quad.m_group; i != -1;) {
        QuadGroup& group = m_groups[i];
        i = group.m_parent;
        x += group.m_x;
    }
    return x;
}

QuadMesh* QuadManager::getMesh(int mesh) {
    return &m_meshes[mesh];
}

bool QuadManager::getMeshVisible(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    if (quad.m_node == NULL) {
        return false;
    }
    return quad.m_node->isRenderingEnabled();
}

void QuadManager::setGroupPosition(int group, float x, float y) {
    QuadGroup& quads = m_groups[group];
    if (quads.m_group == NULL) {
        return;
    }
    quads.m_group->setTranslation(x, -y, 0.0f);
    quads.m_x = (int)x;
    quads.m_y = (int)-y;
}

void QuadManager::setMeshVisible(int mesh, bool visible) {
    if (mesh == -1) {
        return;
    }
    QuadMesh& quad = m_meshes[mesh];
    if (quad.m_node != NULL) {
        quad.m_node->setRenderingEnable(visible);
    }
}

void QuadManager::setGroupVisible(int group, bool visible) {
    if (group == -1) {
        return;
    }
    QuadGroup& quads = m_groups[group];
    if (quads.m_group != NULL) {
        quads.m_group->setRenderingEnable(visible);
    }
}

void QuadManager::setAnimTime(int anim, int time) {
    m_animPlayers[anim]->snapToTime(time);
}

void QuadManager::setAnimFrame(int anim, int frame) {
    m_animPlayers[anim]->snapToFrame(frame);
}

void QuadManager::playAnimAcross(int anim, int mode, int from, int to) {
    m_animPlayers[anim]->playAnimAcross(mode, from, to);
}

void QuadManager::playAnimTo(int anim, int frame) {
    if (anim == -1) {
        return;
    }
    m_animPlayers[anim]->playAnimTo(frame);
}

void QuadManager::playAnimReverse(int anim, int mode) {
    m_animPlayers[anim]->playAnimReverse(mode);
}

void QuadManager::playAnim(int anim, int mode) {
    m_animPlayers[anim]->playAnim(mode);
}

void QuadManager::render(m3g::Graphics3D* g3d, int scope) {
    m_camera->setScope(scope);
    threadLock();
    g3d->render(m_world);
    threadUnlock();
}

void QuadManager::render(m3g::Graphics3D* g3d) {
    m_camera->setScope(SCOPE_DEFAULT);
    threadLock();
    g3d->render(m_world);
    threadUnlock();
}

void QuadManager::render(midp::Graphics* g, int scope) {
    m3g::Graphics3D* g3d = AppEngine::getCanvas()->getGraphics3D();
    threadLock();
    g3d->bindTarget(g);
    m_camera->setScope(scope);
    g3d->render(m_world);
    g3d->releaseTarget();
    threadUnlock();
}

void QuadManager::render(midp::Graphics* g) {
    m3g::Graphics3D* g3d = AppEngine::getCanvas()->getGraphics3D();
    threadLock();
    g3d->bindTarget(g);
    m_camera->setScope(SCOPE_DEFAULT);
    g3d->render(m_world);
    g3d->releaseTarget();
    threadUnlock();
}

void QuadManager::setViewport(int x, int y, int width, int height, int rotation) {
    m_camera->setParallel((float)width, (float)height / (float)width, CAMERA_NEAR, CAMERA_FAR);
    switch (rotation) {
    case 0:
    case 2:
        m_camera->setTranslation((float)(height >> 1), (float)-(width >> 1), 0.0f);
        break;
    case 1:
        m_camera->setTranslation((float)(width >> 1), (float)-(height >> 1), 0.0f);
        break;
    }
    m_camera->setOrientation((float)rotation * DEGREES_PER_ROTATION, 0.0f, 0.0f, 1.0f);
#ifdef MEDECOMP_SPLIT_STACKED
    if (rotation == 1) {
        const int extra = medecomp_wide_extra();
        m_camera->setParallel((float)(width + 2 * extra), (float)height / (width + 2 * extra), CAMERA_NEAR, CAMERA_FAR);
    }
#endif
}

void QuadManager::clearViewport() {
    AppEngine* canvas = AppEngine::getCanvas();
    setViewport(0, 0, canvas->getWidth(), canvas->getHeight(), 1);
#ifdef MEDECOMP_PORT
    const int extra = medecomp_wide_extra();
    m_camera->setParallel((float)(canvas->getWidth() + 2 * extra), (float)canvas->getHeight() / (canvas->getWidth() + 2 * extra),
                          CAMERA_NEAR, CAMERA_FAR);
    m_camera->setTranslation((float)(canvas->getWidth() >> 1), (float)-(canvas->getHeight() >> 1), 0.0f);
#endif
}

void QuadManager::updateAnim(int anim, int dt) {
    if (anim != -1 && m_animPlayers != NULL) {
        m_animPlayers[anim]->update(dt);
    }
}

void QuadManager::updateMesh(QuadMesh* mesh) {
    m3g::Mesh* node = mesh->m_node;
    if (node == NULL) {
        return;
    }
    float x = mesh->m_x + (float)mesh->m_attribs[QuadMesh::ATTRIB_X] * FIXED_TO_FLOAT;
    float y = mesh->m_y + (float)mesh->m_attribs[QuadMesh::ATTRIB_Y] * FIXED_TO_FLOAT;
    float angle = mesh->m_attribs[QuadMesh::ATTRIB_ROTATION];
    threadLock();
    node->setOrientationx((int)angle, 0, 0, 0x10000);
    node->setTranslation(x, -y, 0.0f);
    node->setScalex(mesh->m_attribs[QuadMesh::ATTRIB_WIDTH], mesh->m_attribs[QuadMesh::ATTRIB_HEIGHT], 0);
    node->setAlphaFactorx(mesh->m_attribs[QuadMesh::ATTRIB_ALPHA]);
    threadUnlock();
    mesh->m_dirty = false;
}

void QuadManager::setMeshAlpha(int mesh, float alpha) {
    QuadMesh* quad = &m_meshes[mesh];
    quad->m_attribs[QuadMesh::ATTRIB_ALPHA] = (int)(alpha * FIXED_ONE);
    updateMesh(quad);
}

void QuadManager::setMeshColor(int mesh, unsigned int color) {
    QuadMesh* quad = &m_meshes[mesh];
    quad->m_color = color | 0xff000000;
    float alpha = (float)(color >> 24);
    if (quad->m_node != NULL) {
        quad->m_node->getVertexBuffer()->setDefaultColor(quad->m_color);
    }
    quad->m_attribs[QuadMesh::ATTRIB_ALPHA] = (int)(alpha * BYTE_TO_UNIT * FIXED_ONE);
    updateMesh(quad);
}

void QuadManager::setMeshRotation(int mesh, float degrees) {
    QuadMesh* quad = &m_meshes[mesh];
    quad->m_attribs[QuadMesh::ATTRIB_ROTATION] = (int)(degrees * FIXED_ONE);
    updateMesh(quad);
}

void QuadManager::setMeshPosition(int mesh, float x, float y, int anchor) {
    QuadMesh* quad = &m_meshes[mesh];
    quad->m_x = x;
    quad->m_y = y;
    if (anchor & ANCHOR_LEFT) {
        quad->m_x += (float)(quad->m_attribs[QuadMesh::ATTRIB_WIDTH] >> 17);
    } else if (anchor & ANCHOR_RIGHT) {
        quad->m_x -= (float)(quad->m_attribs[QuadMesh::ATTRIB_WIDTH] >> 17);
    }
    if (anchor & ANCHOR_TOP) {
        quad->m_y += (float)(quad->m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 17);
    } else if (anchor & ANCHOR_BOTTOM) {
        quad->m_y -= (float)(quad->m_attribs[QuadMesh::ATTRIB_HEIGHT] >> 17);
    }
    updateMesh(quad);
}

void QuadManager::setMeshBounds(int mesh, float x, float y, float width, float height, int anchor) {
    QuadMesh& quad = m_meshes[mesh];
    quad.m_attribs[QuadMesh::ATTRIB_WIDTH] = (int)(width * FIXED_ONE);
    quad.m_attribs[QuadMesh::ATTRIB_HEIGHT] = (int)(height * FIXED_ONE);
    setMeshPosition(mesh, x, y, anchor);
}

void QuadManager::freeMesh(int mesh) {
    QuadMesh& quad = m_meshes[mesh];
    if (quad.m_node == NULL) {
        return;
    }
    m3g::Group* group = m_groups[quad.m_group].m_group;
    threadLock();
    group->removeChild(quad.m_node);
    threadUnlock();
    quad.m_node = NULL;
    if (quad.m_texture != -1) {
        freeTexture(quad.m_texture);
    }
}

void QuadManager::freeQuads(int group) {
    QuadGroup& quads = m_groups[group];
    if (quads.m_group == NULL) {
        return;
    }
    int first = quads.m_firstGroup;
    int count = quads.m_numGroups;
    for (int i = 0; i != count; i++) {
        freeQuads(first + i);
    }
    first = quads.m_firstMesh;
    count = quads.m_numMeshes;
    for (int i = 0; i != count; i++) {
        freeMesh(first + i);
    }
    threadLock();
    m3g::Group* parent = quads.m_parent == -1 ? m_world : m_groups[quads.m_parent].m_group;
    parent->removeChild(quads.m_group);
    threadUnlock();
    quads.m_group = NULL;
}

void QuadManager::removeRawChild(m3g::Node* node) {
    threadLock();
    m_world->removeChild(node);
    threadUnlock();
}

void QuadManager::addRawChild(m3g::Node* node) {
    threadLock();
    M3GAssets::addNode(m_world, node);
    threadUnlock();
}

void QuadManager::ensureTextureLoaded(QuadTexture& texture) {
    if (texture.m_texture != NULL) {
        return;
    }
    QuadImage& image = m_images[texture.m_image];
    if (image.m_image == NULL) {
        image.m_image = AppEngine::getCanvas()->getResourceManager().loadM3GImage2D(image.m_resource);
        if (image.m_texScale == 0.0f) {
            int width = image.m_image->getWidth();
            int height = image.m_image->getHeight();
            int size = midp::Math::max(width, height);
            image.m_texScale = 1.0f / (float)size;
            image.m_size = size;
            image.m_scaleU = size / width;
            image.m_scaleV = size / height;
        }
    }

    m3g::Texture2D* texture2D = new m3g::Texture2D(image.m_image);
    image.m_refCount++;
    texture2D->setFiltering(TEXTURE_FILTER_NEAREST, texture.m_imageFilter);
    midp::INCREF(texture2D);
    texture.m_texture = texture2D;

    m3g::VertexArray* texCoords = new m3g::VertexArray(QUAD_VERTICES, 2, VERTEX_TYPE_SHORT);
    short u0 = 0;
    short u1 = image.m_size;
    short v0 = 0;
    short v1 = image.m_size;
    if (texture.m_width != 0) {
        int left = texture.m_x * image.m_scaleU;
        if (texture.m_width > 0) {
            u0 = left;
            u1 = (texture.m_x + texture.m_width) * image.m_scaleU;
        } else {
            u0 = left - texture.m_width * image.m_scaleU;
            u1 = left;
        }
    }
    if (texture.m_height != 0) {
        int top = image.m_size - texture.m_y * image.m_scaleV;
        int height = texture.m_height * image.m_scaleV;
        if (texture.m_height > 0) {
            v0 = top - height;
            v1 = top;
        } else {
            v0 = top;
            v1 = top + height;
        }
    }
    short coords[QUAD_VERTICES * 2] = {u0, v0, u1, v0, u0, v1, u1, v1};
    texCoords->set(0, QUAD_VERTICES, coords);
    midp::INCREF(texCoords);
    texture.m_texCoords = texCoords;
}

void QuadManager::loadMesh(int mesh) {
    midp::array<float> noBias;
    QuadMesh& quad = m_meshes[mesh];
    m3g::Appearance* appearance = new m3g::Appearance();
    if (quad.m_texture < 0) {
        appearance->setCompositingMode(AppEngine::getM3GAssets()->getCompositingMode(COMPOSITING_UNTEXTURED));
    } else {
        QuadTexture& texture = m_textures[quad.m_texture];
        appearance->setCompositingMode(m_compositingModes[texture.m_blending]);
        ensureTextureLoaded(texture);
        texture.m_refCount++;
        appearance->setTexture(0, texture.m_texture);
    }
    appearance->setLayer(quad.m_layer);
    appearance->setPolygonMode(m_polygonMode);

    m3g::VertexBuffer* vertices = new m3g::VertexBuffer();
    vertices->setDefaultColor(quad.m_color);
    vertices->setPositions(m_positions, QUAD_POSITION_SCALE, noBias);
    if (quad.m_texture >= 0) {
        QuadTexture& texture = m_textures[quad.m_texture];
        vertices->setTexCoords(0, texture.m_texCoords, m_images[texture.m_image].m_texScale, noBias);
    }
    quad.m_vertexBuffer = vertices;

    m3g::Mesh* node = new m3g::Mesh(quad.m_vertexBuffer, m_indices, appearance);
    quad.m_node = node;
    node->setScope(quad.m_scope);
    threadLock();
    m_groups[quad.m_group].m_group->addChild(node);
    threadUnlock();
    node->setRenderingEnable(quad.m_visible);
    updateMesh(&quad);
}

void QuadManager::loadGroup(QuadGroup& group) {
    group.m_group = new m3g::Group();
    group.m_group->setRenderingEnable(group.m_visible);
    if (group.m_parent == -1) {
        threadLock();
        m_world->addChild(group.m_group);
    } else {
        QuadGroup& parent = m_groups[group.m_parent];
        if (parent.m_group == NULL) {
            loadGroup(parent);
        }
        threadLock();
        parent.m_group->addChild(group.m_group);
    }
    threadUnlock();
}

void QuadManager::loadQuads(int group) {
    QuadGroup& quads = m_groups[group];
    if (quads.m_group == NULL) {
        loadGroup(quads);
    }
    int first = quads.m_firstMesh;
    int count = quads.m_numMeshes;
    for (int i = 0; i != count; i++) {
        if (m_meshes[first + i].m_node == NULL) {
            loadMesh(first + i);
        }
    }
    first = quads.m_firstGroup;
    count = quads.m_numGroups;
    for (int i = 0; i != count; i++) {
        loadQuads(first + i);
    }
}

void QuadManager::loadQuadData(midp::InputStream* stream, int width, int height) {
    static const short posVertices[] = {
        -1, -1, 0,
         1, -1, 0,
        -1,  1, 0,
         1,  1, 0,
    };

    midp::DataInputStream in(stream);

    int numModes = in.readByte();
    m_compositingModes = midp::array<m3g::CompositingMode*>(numModes);
    for (int i = 0; i != numModes; i++) {
        m3g::CompositingMode* mode = new m3g::CompositingMode();
        mode->setAlphaThresholdx(0);
        mode->setDepthTestEnable(false);
        mode->setDepthWriteEnable(false);
        mode->setBlending(QUAD_BLENDING[i]);
        m_compositingModes[i] = mode;
        midp::INCREF(mode);
    }

    int numImages = in.readShort();
    m_images = midp::array<QuadImage>(numImages);
    for (int i = 0; i != numImages; i++) {
        QuadImage& image = m_images[i];
        image.m_resource = QUAD_TEXTURE[in.readShort()];
    }

    int numTextures = in.readShort();
    m_textures = midp::array<QuadTexture>(numTextures);
    for (int i = 0; i != numTextures; i++) {
        QuadTexture& texture = m_textures[i];
        texture.m_image = in.readShort();
        texture.m_blending = in.readShort();
        texture.m_imageFilter = QUAD_FILTER[in.readShort()];
        texture.m_x = in.readShort();
        texture.m_y = in.readShort();
        texture.m_width = in.readShort();
        texture.m_height = in.readShort();
    }

    int numGroups = in.readShort();
    m_groups = midp::array<QuadGroup>(numGroups);
    for (int i = 0; i != numGroups; i++) {
        QuadGroup& group = m_groups[i];
        group.m_parent = in.readShort();
        group.m_visible = in.readByte() != 0;
        group.m_firstGroup = in.readShort();
        group.m_numGroups = in.readShort();
        group.m_firstMesh = in.readShort();
        group.m_numMeshes = in.readShort();
    }

    int numMeshes = in.readShort();
    m_meshes = midp::array<QuadMesh>(numMeshes);
    for (int i = 0; i != numMeshes; i++) {
        QuadMesh& mesh = m_meshes[i];
        mesh.m_group = in.readShort();
        mesh.m_texture = in.readShort();
        if ((unsigned short)mesh.m_texture != NO_TEXTURE) {
            mesh.m_textureEntry = &m_textures[mesh.m_texture];
        }
        mesh.m_scope = in.readShort();
        mesh.m_layer = in.readShort();
        int red = in.readUnsignedByte();
        int green = in.readUnsignedByte();
        int blue = in.readUnsignedByte();
        mesh.m_color = blue | red << 16 | green << 8 | 0xff000000;
        mesh.m_attribs[QuadMesh::ATTRIB_X] = in.readShort() << 16;
        mesh.m_attribs[QuadMesh::ATTRIB_Y] = in.readShort() << 16;
        int meshWidth = in.readShort();
        bool fullWidth = meshWidth < 0;
        if (meshWidth < 0) {
            meshWidth += width + 1;
#ifdef MEDECOMP_PORT
            meshWidth += 2 * medecomp_wide_extra();
#endif
        }
        mesh.m_attribs[QuadMesh::ATTRIB_WIDTH] = meshWidth << 16;
        int meshHeight = in.readShort();
        if (meshHeight < 0) {
            meshHeight += height + 1;
        }
        mesh.m_attribs[QuadMesh::ATTRIB_HEIGHT] = meshHeight << 16;
        mesh.m_attribs[QuadMesh::ATTRIB_ROTATION] = in.readShort() << 16;
        mesh.m_attribs[QuadMesh::ATTRIB_ALPHA] = in.readInt();

        mesh.m_x = (float)mesh.m_attribs[QuadMesh::ATTRIB_X] * FIXED_TO_FLOAT;
        mesh.m_attribs[QuadMesh::ATTRIB_X] = 0;
        mesh.m_y = (float)mesh.m_attribs[QuadMesh::ATTRIB_Y] * FIXED_TO_FLOAT;
        mesh.m_attribs[QuadMesh::ATTRIB_Y] = 0;

        int anchor = in.readByte();
        if (anchor & ANCHOR_HCENTER) {
            mesh.m_x += (float)(width >> 1);
        } else if (anchor & ANCHOR_RIGHT) {
            mesh.m_x += (float)width;
        }
        if (anchor & ANCHOR_VCENTER) {
            mesh.m_y += (float)(height >> 1);
        } else if (anchor & ANCHOR_BOTTOM) {
            mesh.m_y += (float)height;
        }
#ifdef MEDECOMP_PORT
        if (fullWidth && !(anchor & (ANCHOR_HCENTER | ANCHOR_RIGHT))) mesh.m_x -= medecomp_wide_extra();
#else
        (void)fullWidth;
#endif
        mesh.m_visible = in.readByte() != 0;

        if (meshWidth & 1) {
            mesh.m_x -= 0.5f;
        }
        if (meshHeight & 1) {
            mesh.m_y -= 0.5f;
        }
    }

    int numAnims = in.readShort();
    m_animPlayers = midp::array<AnimPlayerQuad*>(numAnims);
    for (int i = 0; i != numAnims; i++) {
        int id = in.readShort();
        AnimPlayerQuad* anim = new AnimPlayerQuad(this, id);
        m_animPlayers[i] = anim;
        int numAttribs = in.readByte();
        anim->setNumAttributes(numAttribs);
        for (int a = 0; a != numAttribs; a++) {
            QuadMesh* mesh = &m_meshes[in.readShort()];
            int meshAttrib = in.readByte();
            anim->setupAttribute(a, mesh, meshAttrib);
        }
        int numKeyframes = in.readByte();
        anim->setNumKeyframes(numKeyframes);
        for (int k = 0; k != numKeyframes; k++) {
            anim->setKeyframeTime(k, in.readInt());
            for (int a = 0; a != numAttribs; a++) {
                anim->setAttribValue(a, k, in.readInt());
            }
        }
    }
    in.close();

    if (m_polygonMode == NULL) {
        m_polygonMode = new m3g::PolygonMode();
        m_polygonMode->setCulling(POLYGON_CULL_NONE);
        midp::INCREF(m_polygonMode);
    }
    if (m_positions == NULL) {
        m_positions = new m3g::VertexArray(QUAD_VERTICES, 3, VERTEX_TYPE_SHORT);
        m_positions->set(0, QUAD_VERTICES, posVertices);
        midp::INCREF(m_positions);
    }
    if (m_indices == NULL) {
        midp::array<int> stripLengths(1);
        stripLengths[0] = QUAD_VERTICES;
        m_indices = new m3g::TriangleStripArray(0, stripLengths);
        midp::INCREF(m_indices);
    }
}

QuadManager::~QuadManager() {
    if (m_animPlayers != NULL) {
        for (int i = m_animPlayers.m_length; i != 0;) {
            i--;
            free(m_animPlayers[i]);
        }
    }
    for (int i = 0; i != m_compositingModes.m_length; i++) {
        midp::DECREF(m_compositingModes[i]);
    }
    midp::DECREF(m_world);
    midp::DECREF(m_polygonMode);
    midp::DECREF(m_positions);
    midp::DECREF(m_indices);
}

QuadManager::QuadManager()
    : m_locked(false),
      m_world(NULL),
      m_camera(NULL),
      m_polygonMode(NULL),
      m_positions(NULL),
      m_indices(NULL) {
    m_world = new m3g::World();
    midp::INCREF(m_world);
    m_camera = new m3g::Camera();
    m_camera->setParallel(1024.0f, 0.75f, CAMERA_NEAR, CAMERA_FAR);
#ifdef MEDECOMP_PORT
    m_camera->setParallel(1024.0f + 2 * medecomp_wide_extra(), 768.0f / (1024.0f + 2 * medecomp_wide_extra()), CAMERA_NEAR,
                          CAMERA_FAR);
#endif
    m_camera->setOrientation(DEGREES_PER_ROTATION, 0.0f, 0.0f, 1.0f);
    m_camera->setTranslation(512.0f, -384.0f, 0.0f);
    m_world->addChild(m_camera);
    m_world->setActiveCamera(m_camera);
    m3g::Background* background = new m3g::Background();
    background->setColorClearEnable(false);
    background->setDepthClearEnable(false);
    m_world->setBackground(background);
}

void QuadManager::threadUnlock() {
#ifdef MEDECOMP_PORT
    __atomic_store_n(&m_locked, false, __ATOMIC_RELEASE);
#else
    m_locked = false;
#endif
}

void QuadManager::threadLock() {
#ifdef MEDECOMP_PORT
    while (__atomic_exchange_n(&m_locked, true, __ATOMIC_ACQUIRE)) {
        midp::Thread::yield();
    }
#else
    while (m_locked) {
        midp::Thread::yield();
    }
    m_locked = true;
#endif
}

QuadTexture* QuadManager::getTexture(int texture) {
    return &m_textures[texture];
}

bool QuadManager::isPointWithinMesh(int mesh, int x, int y) {
    return isPointWithinMesh(mesh, x, y, 0, 0);
}
