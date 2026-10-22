#pragma once

namespace midp {
class InputStream;
class Image;
template <class T>
class array;
}
namespace m3g {
class Image2D;
class Object3D;
}

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    static midp::InputStream* loadBinaryFile(int id);
    midp::array<m3g::Object3D*> loadM3GFile(int id);
    m3g::Object3D* loadM3GNode(int id);
    m3g::Image2D* loadM3GImage2D(int id);

    static const int SOUND_TRACK_SOUNDS[];
    static const int MATERIAL_LOOKUP[];
    midp::Image* loadImage(int id) const;
    static const short IMAGE_RES_IDS[];
    static const int RESOURCE_FILESIZE_LIST[];
    static const int ANIMATION_CONTROLLER_LOOKUP[];
    static const char RESOURCE_FILENAMES_LIST[][40];
    static const int SOUND_DATA_SETS[];
    static const int SOUND_EVENT_LOOKUP[];
    static const int SOUND_TRACK_POOL_LOOKUPS[];
    static const int LOADING_IMAGE_LOOKUP[];
    static const int LOADING_STRING_LOOKUP[];
    static const int LOADING_FONT_LOOKUP[];
    static const int UNLOCKABLE_QUAD_DATA[];
    static const int UNLOCKABLE_SAVEABLE_DATA[];
    static const int UNLOCKABLE_STRING_DATA[];

    bool m_unk00;
};
