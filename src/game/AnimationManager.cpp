#include "game/AnimationManager.h"

#include <stddef.h>

#include "game/AnimPlayer.h"
#include "game/AnimationManagerData.h"
#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/InputStream.h"

static const int RESOURCE_ANIMS = 184;
static const int RESOURCE_SUBIMAGES = 199;
static const int RESOURCE_COLORS = 197;

static const int NUM_IMAGES = 1;

static const int TRANS_NONE = 0;
static const int TRANS_MIRROR = 2;

static const int IMAGE_ANCHOR = 9;

static int findImage(int id) {
    const short* ids = ResourceManager::IMAGE_RES_IDS;
    int i;
    for (i = 0; i < NUM_IMAGES; i++) {
        if (ids[i] == id) {
            break;
        }
    }
    return i;
}

void AnimationManager::constructAnimationManager(AnimationManagerData& data) {
    for (int i = 0; i < AnimationManagerData::NUM_PLAYERS; i++) {
        data.m_players[i] = new AnimPlayer;
    }
    data.m_colors = midp::array<signed char>();
    data.m_frameCounts = midp::array<signed char>(AnimationManagerData::NUM_ANIMS);
    data.m_firstFrame = midp::array<short>(AnimationManagerData::NUM_ANIMS);
    data.m_frameDurations = midp::array<short>(AnimationManagerData::NUM_FRAMES);
    data.m_primitiveCounts = midp::array<signed char>(AnimationManagerData::NUM_FRAMES);
    data.m_firstPrimitive = midp::array<int>(AnimationManagerData::NUM_FRAMES);
    data.m_primitives = midp::array<short>(AnimationManagerData::NUM_PRIMITIVES);
    data.m_images = midp::array<midp::array<midp::Image*> >(1);
    data.m_images[0] = midp::array<midp::Image*>(NUM_IMAGES);
}

int AnimationManager::getNumAttributes(int type) {
    switch (type) {
    case PRIM_IMAGE:
    case PRIM_IMAGE_MIRRORED:
        return 4;
    case PRIM_CLIP:
    case PRIM_UNK2:
    case PRIM_RECT:
    case PRIM_FILL_RECT:
    case PRIM_ELLIPSE:
    case PRIM_FILL_ELLIPSE:
    case PRIM_LINE:
    case PRIM_FILL_TRIANGLE:
        return 5;
    case PRIM_UNK7:
        return 3;
    case PRIM_COLOR:
        return 2;
    default:
        return 0;
    }
}

int AnimationManager::getAnimFrameDuration(int anim, int frame) {
    AnimationManagerData& data = AppEngine::getCanvas()->getAnimManData();
    return data.m_frameDurations[data.m_firstFrame[anim & ~ANIM_ID_FLAG] + frame];
}

int AnimationManager::getAnimFrameCount(int anim) {
    AppEngine* engine = AppEngine::getCanvas();
    anim &= ~ANIM_ID_FLAG;
    if ((unsigned)anim >= AnimationManagerData::NUM_ANIMS) {
        return -1;
    }
    return engine->getAnimManData().m_frameCounts[anim];
}

bool AnimationManager::unloadImage(int id) {
    AnimationManagerData& data = AppEngine::getCanvas()->getAnimManData();
    int i = findImage(id);
    if (i == NUM_IMAGES) {
        return false;
    }
    midp::Image* image = data.m_images[0][i];
    data.m_images[0][i] = NULL;
    midp::DECREF(image);
    return true;
}

void AnimationManager::setColor(midp::Graphics* g, int color) {
    AnimationManagerData& data = AppEngine::getCanvas()->getAnimManData();
    int i = color * 3;
    unsigned char r = data.m_colors[i];
    unsigned char gr = data.m_colors[i + 1];
    unsigned char b = data.m_colors[i + 2];
    g->setColor(r, gr, b);
}

int AnimationManager::drawPrimitive(midp::Graphics* g, int x, int y, int index, int type) {
    AnimationManagerData& data = AppEngine::getCanvas()->getAnimManData();
    const midp::array<short>& prims = data.m_primitives;
    switch (type) {
    case PRIM_CLIP:
        g->clipRect(x + prims[index + 1], y + prims[index + 2], prims[index + 3], prims[index + 4]);
        break;
    case PRIM_RECT:
    case PRIM_FILL_RECT:
    case PRIM_ELLIPSE:
    case PRIM_FILL_ELLIPSE: {
        int left = x + prims[index + 1];
        int top = y + prims[index + 2];
        int width = prims[index + 3];
        int height = prims[index + 4];
        switch (type) {
        case PRIM_RECT:
            g->drawRect(left, top, width - 1, height - 1);
            break;
        case PRIM_FILL_RECT:
            g->fillRect(left, top, width, height);
            break;
        case PRIM_ELLIPSE:
            g->drawArc(left, top, width, height, 0, 360);
            break;
        case PRIM_FILL_ELLIPSE:
            g->fillArc(left, top, width, height, 0, 360);
            break;
        }
        break;
    }
    case PRIM_LINE:
        g->drawLine(x + prims[index + 1], y + prims[index + 2], x + prims[index + 3], y + prims[index + 4]);
        break;
    case PRIM_COLOR:
        setColor(g, prims[index + 1]);
        break;
    case PRIM_FILL_TRIANGLE: {
        int x1 = x + prims[1];
        int y1 = y + prims[2];
        int x2 = x + prims[3];
        int y2 = y + prims[4];
        g->fillTriangle(x1, y1, x2, y2, x1, y2);
        break;
    }
    default:
        break;
    }
    return 0;
}

void AnimationManager::drawNormalAnimFrame(midp::Graphics* g, int anim, int frame, int x, int y) {
    AnimationManagerData& data = AppEngine::getCanvas()->getAnimManData();
    int frameIndex = data.m_firstFrame[anim] + frame;
    int count = data.m_primitiveCounts[frameIndex];
    if (count == 0) {
        return;
    }
    int index = data.m_firstPrimitive[frameIndex];
    const midp::array<short>& prims = data.m_primitives;
    do {
        int type = prims[index];
        if (type == PRIM_IMAGE || type == PRIM_IMAGE_MIRRORED) {
            int left = x + prims[index + 1];
            int top = y + prims[index + 2];
            const AnimationManagerData::Subimage& sub = data.m_subimages[prims[index + 3]];
            midp::Image* image = data.m_images[data.m_imageSet][sub.image];
            g->drawRegion(image, sub.x, sub.y, sub.width, sub.height,
                          type == PRIM_IMAGE_MIRRORED ? TRANS_MIRROR : TRANS_NONE, left, top, IMAGE_ANCHOR);
        } else if (type != PRIM_UNK2 && type != PRIM_UNK7) {
            drawPrimitive(g, x, y, index, type);
        }
        index += getNumAttributes(type);
    } while (--count != 0);
}

void AnimationManager::drawAnimFrame(midp::Graphics* g, int anim, int frame, int x, int y) {
    if (anim >= 0) {
        drawNormalAnimFrame(g, anim, frame, x, y);
    }
}

bool AnimationManager::loadImage(AnimationManagerData& data, ResourceManager& resources, int id) {
    int i = findImage(id);
    if (i == NUM_IMAGES) {
        return false;
    }
    if (data.m_images[0][i] == NULL) {
        midp::Image* image = resources.loadImage(id);
        midp::INCREF(image);
        data.m_images[0][i] = image;
    }
    return true;
}

bool AnimationManager::loadImage(ResourceManager& resources, int id) {
    return loadImage(AppEngine::getCanvas()->getAnimManData(), resources, id);
}

bool AnimationManager::loadAnimFile(AnimationManagerData& data, ResourceManager& resources) {
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_ANIMS));
    midp::array<short>& prims = data.m_primitives;
    int index = 0;
    int firstFrame = 0;
    for (int anim = 0; anim < AnimationManagerData::NUM_ANIMS; anim++) {
        int frames = in.readUnsignedByte();
        data.m_frameCounts[anim] = frames;
        data.m_firstFrame[anim] = firstFrame;
        if (frames == 0) {
            continue;
        }
        int frame = firstFrame;
        for (int f = frames; f != 0; f--, frame++) {
            data.m_frameDurations[frame] = in.readShort();
            int count = in.readUnsignedByte();
            data.m_primitiveCounts[frame] = count;
            data.m_firstPrimitive[frame] = index;
            for (; count != 0; count--) {
                int type = in.readUnsignedByte();
                prims[index] = type;
                switch (type) {
                case PRIM_IMAGE:
                case PRIM_IMAGE_MIRRORED:
                    prims[index + 3] = in.readShort();
                    prims[index + 1] = in.readShort();
                    prims[index + 2] = in.readShort();
                    index += 4;
                    break;
                case PRIM_CLIP:
                case PRIM_UNK2:
                case PRIM_RECT:
                case PRIM_FILL_RECT:
                case PRIM_ELLIPSE:
                case PRIM_FILL_ELLIPSE:
                case PRIM_LINE:
                case PRIM_FILL_TRIANGLE:
                    prims[index + 1] = in.readShort();
                    prims[index + 2] = in.readShort();
                    prims[index + 3] = in.readShort();
                    prims[index + 4] = in.readShort();
                    index += 5;
                    break;
                case PRIM_UNK7:
                    prims[index + 1] = in.readShort();
                    prims[index + 2] = in.readShort();
                    index += 3;
                    break;
                case PRIM_COLOR:
                    prims[index + 1] = in.readUnsignedByte();
                    index += 2;
                    break;
                default:
                    index += 1;
                    break;
                }
            }
        }
        firstFrame += frames;
    }
    return true;
}

bool AnimationManager::loadSubimageFile(AnimationManagerData& data, ResourceManager& resources) {
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_SUBIMAGES));
    for (int i = 0; i < AnimationManagerData::NUM_SUBIMAGES; i++) {
        AnimationManagerData::Subimage& sub = data.m_subimages[i];
        sub.image = in.readUnsignedShort();
        sub.x = in.readUnsignedShort();
        sub.y = in.readUnsignedShort();
        sub.width = in.readUnsignedShort();
        sub.height = in.readUnsignedShort();
    }
    return true;
}

bool AnimationManager::loadColorsFile(AnimationManagerData& data, ResourceManager& resources) {
    midp::InputStream* in = ResourceManager::loadBinaryFile(RESOURCE_COLORS);
    data.m_colors = midp::array<signed char>(ResourceManager::RESOURCE_FILESIZE_LIST[RESOURCE_COLORS]);
    in->read(data.m_colors);
    in->close();
    delete in;
    return true;
}
