#pragma once

#include "runtime/midp/array.h"

namespace m3g {
class AppearanceBase;
class Node;
class Group;
class Appearance;
class CompositingMode;
class PolygonMode;
class Image2D;
class Texture2D;
}
class M3GAssets_Image;
class M3GAssets_Texture;
class M3GAssets_TexGroup;
class M3GAssets_AppGroup;
class M3GAssets_File;
class M3GAssets_Node;
class M3GAssets_Model;

class M3GAssets {
public:
    M3GAssets();
    ~M3GAssets();

    void loadData();
    void freeCaches(int flags);

    m3g::Node* loadNode(int id);
    m3g::Node* loadNodeCached(int id, int flags);
    m3g::Node* loadModel(int id, int flags);
    const M3GAssets_AppGroup& loadTextureGroup(int id, int flags);
    m3g::AppearanceBase* loadTexturedAppearance(int id, int flags);
    m3g::Appearance* getTexturedAppearance(int id);
    m3g::Appearance* getAppearance(int index);
    m3g::CompositingMode* getCompositingMode(int index);

    static void orphanNode(m3g::Node* node);
    static void addNode(m3g::Group* group, m3g::Node* node);
    static void cacheSkinTransforms(m3g::Node* node);
    static void commit(m3g::Node* node);
    static void applyAlphaFactor(m3g::Node* node, float alpha);
    static void applyColor(m3g::Node* node, int color);
    static void applyAppearanceGroup(m3g::Node* node, const M3GAssets_AppGroup& group);
    static void applyAppearance(m3g::Node* node, m3g::Appearance* appearance);

    static m3g::Texture2D* createTexture2D(m3g::Image2D* image, int levelFilter, int imageFilter,
                                           int blending);
    static m3g::Appearance* createTexturedAppearance(m3g::Appearance* base, m3g::Image2D* image,
                                                     int levelFilter, int imageFilter, int blending);
    static m3g::Appearance* createAppearanace(m3g::CompositingMode* compositing,
                                              m3g::PolygonMode* polygon, int layer);
    static m3g::CompositingMode* createCompositingMode(int flags, float alphaThreshold,
                                                       int blending);
    static m3g::PolygonMode* createPolygonMode(int culling);

    static const short RES_ID_ARRAY[105];
    static const short BLENDING_ARRAY[3];
    static const short CULLING_TYPE_ARRAY[2];
    static const short TEXTURE_FILTER_ARRAY[2];
    static const short TEXTURE_BLENDING_ARRAY[2];

    midp::array<m3g::PolygonMode*> m_polygonModes;
    midp::array<m3g::CompositingMode*> m_compositingModes;
    midp::array<m3g::Appearance*> m_appearances;
    midp::array<M3GAssets_Image*> m_images;
    midp::array<M3GAssets_Texture*> m_textures;
    midp::array<M3GAssets_TexGroup*> m_texGroups;
    midp::array<M3GAssets_File*> m_files;
    midp::array<M3GAssets_Node*> m_nodes;
    midp::array<M3GAssets_Model*> m_models;
};
