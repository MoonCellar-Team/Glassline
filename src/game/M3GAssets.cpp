#include "game/M3GAssets.h"

#include "game/AppEngine.h"
#include "game/M3GAssets_AppGroup.h"
#include "game/M3GAssets_File.h"
#include "game/M3GAssets_Image.h"
#include "game/M3GAssets_Model.h"
#include "game/M3GAssets_Node.h"
#include "game/M3GAssets_TexGroup.h"
#include "game/M3GAssets_Texture.h"
#include "game/ResourceManager.h"
#include "m3g/LoaderHelpers.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_M3G_ASSETS = 202;

const short M3GAssets::RES_ID_ARRAY[105] = {
    -1,  176, 175, 101, 103, 104, 301, 300, 254, 255, 178, 177, 179, 180, 4,   5,   1,   38,
    0,   3,   2,   36,  37,  144, 146, 148, 147, 79,  75,  80,  76,  78,  81,  77,  72,  132,
    40,  128, 127, 73,  41,  103, 87,  94,  109, 110, 86,  93,  107, 108, 92,  99,  111, 112,
    100, 101, 102, 104, 105, 106, 113, 114, 115, 95,  88,  96,  89,  97,  90,  98,  91,  84,
    85,  125, 82,  126, 20,  24,  23,  25,  27,  31,  28,  40,  33,  34,  35,  41,  32,  42,
    43,  37,  38,  30,  26,  39,  25,  27,  33,  30,  31,  34,  26,  28,  44,
};
const short M3GAssets::BLENDING_ARRAY[3] = {68, 64, 65};
const short M3GAssets::CULLING_TYPE_ARRAY[2] = {162, 160};
const short M3GAssets::TEXTURE_FILTER_ARRAY[2] = {209, 210};
const short M3GAssets::TEXTURE_BLENDING_ARRAY[2] = {228, 227};

m3g::Appearance* M3GAssets::getAppearance(int index) {
    return m_appearances[index];
}

m3g::Appearance* M3GAssets::getTexturedAppearance(int id) {
    return m_textures[id]->get();
}

void M3GAssets::orphanNode(m3g::Node* node) {
    if (node == NULL) {
        return;
    }
    m3g::Node* parent = node->getParent();
    if (parent == NULL) {
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(parent);
    if (group != NULL) {
        group->removeChild(node);
    }
}

void M3GAssets::addNode(m3g::Group* group, m3g::Node* node) {
    midp::INCREF(node);
    orphanNode(node);
    group->addChild(node);
    midp::DECREF(node);
}

void M3GAssets::freeCaches(int flags) {
    for (int i = m_models.m_length - 1; i >= 0; i--) {
        m_models[i]->freeCached(flags);
    }
    for (int i = m_nodes.m_length - 1; i >= 0; i--) {
        m_nodes[i]->freeCached(flags);
    }
    for (int i = m_texGroups.m_length - 1; i >= 0; i--) {
        m_texGroups[i]->freeCached(flags);
    }
    for (int i = m_textures.m_length - 1; i >= 0; i--) {
        m_textures[i]->freeCached(flags);
    }
}

void M3GAssets::cacheSkinTransforms(m3g::Node* node) {
    if (node == NULL) {
        return;
    }
    if (m3g::m3g_cast<m3g::Mesh>(node) != NULL) {
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(node);
        if (skinned != NULL) {
            m3g::Loader_SkinnedMesh_warmCache(skinned);
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group == NULL) {
        return;
    }
    int count = group->getChildCount();
    for (int i = 0; i < count; i++) {
        cacheSkinTransforms(group->getChild(i));
    }
}

void M3GAssets::commit(m3g::Node* node) {
    if (node == NULL) {
        return;
    }
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL) {
        mesh->commit();
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(node);
        if (skinned != NULL) {
            commit(skinned->getSkeleton());
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group == NULL) {
        return;
    }
    int count = group->getChildCount();
    for (int i = 0; i < count; i++) {
        commit(group->getChild(i));
    }
}

void M3GAssets::applyAlphaFactor(m3g::Node* node, float alpha) {
    if (node == NULL) {
        return;
    }
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL) {
        mesh->setAlphaFactor(alpha);
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(mesh);
        if (skinned != NULL) {
            applyAlphaFactor(skinned->getSkeleton(), alpha);
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group == NULL) {
        return;
    }
    int count = group->getChildCount();
    for (int i = 0; i < count; i++) {
        applyAlphaFactor(group->getChild(i), alpha);
    }
}

void M3GAssets::applyColor(m3g::Node* node, int color) {
    if (node == NULL) {
        return;
    }
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL) {
        mesh->getVertexBuffer()->setDefaultColor(color);
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(mesh);
        if (skinned != NULL) {
            applyColor(skinned->getSkeleton(), color);
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group == NULL) {
        return;
    }
    int count = group->getChildCount();
    for (int i = 0; i < count; i++) {
        applyColor(group->getChild(i), color);
    }
}

void M3GAssets::applyAppearanceGroup(m3g::Node* node, const M3GAssets_AppGroup& group) {
    if (node == NULL) {
        return;
    }
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL) {
        int submeshes = mesh->getSubmeshCount();
        for (int i = 0; i < submeshes; i++) {
            int id = mesh->getAppearance(i)->getUserID();
            mesh->setAppearance(i, group.getAppearance(id));
        }
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(mesh);
        if (skinned != NULL) {
            applyAppearanceGroup(skinned->getSkeleton(), group);
        }
        return;
    }
    m3g::Group* children = m3g::m3g_cast<m3g::Group>(node);
    if (children == NULL) {
        return;
    }
    int count = children->getChildCount();
    for (int i = 0; i < count; i++) {
        applyAppearanceGroup(children->getChild(i), group);
    }
}

void M3GAssets::applyAppearance(m3g::Node* node, m3g::Appearance* appearance) {
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL) {
        int submeshes = mesh->getSubmeshCount();
        for (int i = 0; i < submeshes; i++) {
            mesh->setAppearance(i, appearance);
        }
        m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(node);
        if (skinned != NULL) {
            applyAppearance(skinned->getSkeleton(), appearance);
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group == NULL) {
        return;
    }
    int count = group->getChildCount();
    for (int i = 0; i < count; i++) {
        applyAppearance(group->getChild(i), appearance);
    }
}

m3g::Texture2D* M3GAssets::createTexture2D(m3g::Image2D* image, int levelFilter, int imageFilter,
                                           int blending) {
    m3g::Texture2D* texture = new m3g::Texture2D(image);
    texture->setFiltering(levelFilter, imageFilter);
    texture->setBlending(blending);
    return texture;
}

m3g::Appearance* M3GAssets::createTexturedAppearance(m3g::Appearance* base, m3g::Image2D* image,
                                                     int levelFilter, int imageFilter,
                                                     int blending) {
    m3g::Appearance* appearance = static_cast<m3g::Appearance*>(base->duplicate());
    appearance->setTexture(0, createTexture2D(image, levelFilter, imageFilter, blending));
    return appearance;
}

m3g::Appearance* M3GAssets::createAppearanace(m3g::CompositingMode* compositing,
                                              m3g::PolygonMode* polygon, int layer) {
    m3g::Appearance* appearance = new m3g::Appearance();
    appearance->setCompositingMode(compositing);
    appearance->setLayer(layer);
    appearance->setPolygonMode(polygon);
    return appearance;
}

m3g::CompositingMode* M3GAssets::createCompositingMode(int flags, float alphaThreshold,
                                                       int blending) {
    m3g::CompositingMode* mode = new m3g::CompositingMode();
    mode->setDepthTestEnable((flags & 1) != 0);
    mode->setDepthWriteEnable(((flags >> 1) & 1) != 0);
    mode->setColorWriteEnable(((flags >> 2) & 1) != 0);
    mode->setAlphaWriteEnable(((flags >> 3) & 1) != 0);
    mode->setAlphaThreshold(alphaThreshold);
    mode->setBlending(blending);
    return mode;
}

m3g::PolygonMode* M3GAssets::createPolygonMode(int culling) {
    m3g::PolygonMode* mode = new m3g::PolygonMode();
    mode->setCulling(culling);
    return mode;
}

template <class T>
static void deleteAll(midp::array<T*>& items) {
    for (int i = 0; i < items.m_length; i++) {
        delete items[i];
        items[i] = NULL;
    }
    items = midp::array<T*>();
}

template <class T>
static void releaseAll(midp::array<T*>& items) {
    for (int i = 0; i < items.m_length; i++) {
        midp::DECREF(items[i]);
    }
    items = midp::array<T*>();
}

M3GAssets::~M3GAssets() {
    deleteAll(m_models);
    deleteAll(m_nodes);
    deleteAll(m_files);
    deleteAll(m_texGroups);
    deleteAll(m_textures);
    deleteAll(m_images);
    releaseAll(m_appearances);
    releaseAll(m_compositingModes);
    releaseAll(m_polygonModes);
}

M3GAssets::M3GAssets() {}

void M3GAssets::loadData() {
    if (!(m_polygonModes == NULL)) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_M3G_ASSETS));

    int count = in.readUnsignedShort();
    m_polygonModes = midp::array<m3g::PolygonMode*>(count);
    for (int i = 0; i < count; i++) {
        m3g::PolygonMode* mode = createPolygonMode(CULLING_TYPE_ARRAY[in.readShort()]);
        midp::INCREF(mode);
        m_polygonModes[i] = mode;
    }

    count = in.readUnsignedShort();
    m_compositingModes = midp::array<m3g::CompositingMode*>(count);
    for (int i = 0; i < count; i++) {
        int flags = in.readUnsignedByte();
        float alphaThreshold = (float)in.readInt() * (1.0f / 65536.0f);
        int blending = BLENDING_ARRAY[in.readShort()];
        m3g::CompositingMode* mode = createCompositingMode(flags, alphaThreshold, blending);
        midp::INCREF(mode);
        m_compositingModes[i] = mode;
    }

    count = in.readUnsignedShort();
    m_appearances = midp::array<m3g::Appearance*>(count);
    for (int i = 0; i < count; i++) {
        m3g::PolygonMode* polygon = m_polygonModes[in.readShort()];
        m3g::CompositingMode* compositing = m_compositingModes[in.readShort()];
        int layer = in.readShort();
        m3g::Appearance* appearance = createAppearanace(compositing, polygon, layer);
        midp::INCREF(appearance);
        m_appearances[i] = appearance;
    }

    count = in.readUnsignedShort();
    m_images = midp::array<M3GAssets_Image*>(count);
    for (int i = 0; i < count; i++) {
        m_images[i] = new M3GAssets_Image(RES_ID_ARRAY[in.readShort()]);
    }

    count = in.readUnsignedShort();
    m_textures = midp::array<M3GAssets_Texture*>(count);
    for (int i = 0; i < count; i++) {
        m3g::Appearance* base = m_appearances[in.readShort()];
        M3GAssets_Image* image = m_images[in.readShort()];
        int levelFilter = TEXTURE_FILTER_ARRAY[in.readShort()];
        int imageFilter = TEXTURE_FILTER_ARRAY[in.readShort()];
        int blending = TEXTURE_BLENDING_ARRAY[in.readShort()];
        m_textures[i] = new M3GAssets_Texture(base, image, levelFilter, imageFilter, blending);
    }

    count = in.readUnsignedShort();
    m_texGroups = midp::array<M3GAssets_TexGroup*>(count);
    for (int i = 0; i < count; i++) {
        M3GAssets_Texture* defaultTexture = m_textures[in.readShort()];
        signed char elements = in.readByte();
        M3GAssets_TexGroup* group = new M3GAssets_TexGroup(elements, defaultTexture);
        m_texGroups[i] = group;
        for (signed char left = elements; left != 0; left--) {
            M3GAssets_Texture* texture = m_textures[in.readShort()];
            group->addElement(RES_ID_ARRAY[in.readShort()], texture);
        }
    }

    count = in.readUnsignedShort();
    m_files = midp::array<M3GAssets_File*>(count);
    for (int i = 0; i < count; i++) {
        m_files[i] = new M3GAssets_File(RES_ID_ARRAY[in.readShort()]);
    }

    count = in.readUnsignedShort();
    m_nodes = midp::array<M3GAssets_Node*>(count);
    for (int i = 0; i < count; i++) {
        M3GAssets_File* file = m_files[in.readShort()];
        int userId = RES_ID_ARRAY[in.readShort()];
        bool unique = in.readBoolean();
        m_nodes[i] = new M3GAssets_Node(file, userId, unique);
    }

    count = in.readUnsignedShort();
    m_models = midp::array<M3GAssets_Model*>(count);
    for (int i = 0; i < count; i++) {
        M3GAssets_Node* node = m_nodes[in.readShort()];
        bool unique = in.readBoolean();
        bool commitMeshes = in.readBoolean();
        M3GAssets_Texture* texture = NULL;
        if (in.readByte() == 1) {
            texture = m_textures[in.readShort()];
        }
        M3GAssets_TexGroup* texGroup = NULL;
        if (in.readByte() == 1) {
            texGroup = m_texGroups[in.readShort()];
        }
        m_models[i] = new M3GAssets_Model(node, texture, texGroup, unique, commitMeshes);
    }
    in.close();
}

m3g::Node* M3GAssets::loadNodeCached(int id, int flags) {
    return m_nodes[id]->loadUniqueCached(flags);
}

m3g::Node* M3GAssets::loadNode(int id) {
    return m_nodes[id]->loadUnique();
}

m3g::Node* M3GAssets::loadModel(int id, int flags) {
    return m_models[id]->loadCached(flags);
}

const M3GAssets_AppGroup& M3GAssets::loadTextureGroup(int id, int flags) {
    return m_texGroups[id]->loadCached(flags);
}

m3g::AppearanceBase* M3GAssets::loadTexturedAppearance(int id, int flags) {
    return m_textures[id]->loadCached(flags);
}

m3g::CompositingMode* M3GAssets::getCompositingMode(int index) {
    return m_compositingModes[index];
}
