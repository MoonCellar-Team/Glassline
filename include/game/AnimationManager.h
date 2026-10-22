#pragma once

class AnimationManagerData;
class ResourceManager;
namespace midp {
class Graphics;
}

class AnimationManager {
public:
    enum PrimitiveType {
        PRIM_IMAGE = 0,
        PRIM_CLIP = 1,
        PRIM_UNK2 = 2,
        PRIM_RECT = 3,
        PRIM_FILL_RECT = 4,
        PRIM_ELLIPSE = 5,
        PRIM_FILL_ELLIPSE = 6,
        PRIM_UNK7 = 7,
        PRIM_LINE = 8,
        PRIM_COLOR = 10,
        PRIM_IMAGE_MIRRORED = 11,
        PRIM_FILL_TRIANGLE = 12,
    };

    static const int ANIM_ID_FLAG = 0x4000;

    static void constructAnimationManager(AnimationManagerData& data);
    static int getNumAttributes(int type);
    static int getAnimFrameDuration(int anim, int frame);
    static int getAnimFrameCount(int anim);
    static bool unloadImage(int id);
    static void setColor(midp::Graphics* g, int color);
    static int drawPrimitive(midp::Graphics* g, int x, int y, int index, int type);
    static void drawNormalAnimFrame(midp::Graphics* g, int anim, int frame, int x, int y);
    static void drawAnimFrame(midp::Graphics* g, int anim, int frame, int x, int y);
    static bool loadImage(AnimationManagerData& data, ResourceManager& resources, int id);
    static bool loadImage(ResourceManager& resources, int id);
    static bool loadAnimFile(AnimationManagerData& data, ResourceManager& resources);
    static bool loadSubimageFile(AnimationManagerData& data, ResourceManager& resources);
    static bool loadColorsFile(AnimationManagerData& data, ResourceManager& resources);
};
