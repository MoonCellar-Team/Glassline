#pragma once

namespace m3g {
class Appearance;
}
class M3GAssets_Image;

class M3GAssets_Texture {
public:
    M3GAssets_Texture(m3g::Appearance* base, M3GAssets_Image* image, int levelFilter,
                      int imageFilter, int blending);
    ~M3GAssets_Texture();

    m3g::Appearance* load();
    m3g::Appearance* loadCached(int flags);
    m3g::Appearance* loadForInternalAsset();
    m3g::Appearance* get();
    void free();
    void freeCached(int flags);
    void freeInternalAssetReference();

    m3g::Appearance* m_base;
    M3GAssets_Image* m_image;
    m3g::Appearance* m_appearance;
    int m_levelFilter;
    int m_imageFilter;
    int m_blending;
    int m_cacheFlags;
    int m_internalRefs;
};
