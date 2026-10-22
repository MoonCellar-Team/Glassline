#include "game/UI/UnlockableItem.h"

#include "game/Achievement.h"
#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/PhotoPicker.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int GROUP_UNLOCK = 0x29;
static const int MESH_LOCKED = 0x41;
static const int MESH_ANCHOR = QuadManager::ANCHOR_TOP | QuadManager::ANCHOR_LEFT;
static const int TEXT_ONE = 0x95d;
static const int TEXT_MANY = 0x900;
static const int THUMB_INSET = 0x18;
static const int ITEM_WIDTH = 0x130;
static const int ITEM_HEIGHT = 0xf0;
static const float FIT_WIDE = 256.0f;
static const float FIT_TALL = 192.0f;

namespace UI {

void UnlockableItem::render(midp::Graphics* g, int originY, int originX) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    m_thumbX = originX + m_x + THUMB_INSET;
    m_thumbY = originY + m_y + THUMB_INSET;
    int mesh = getQuadId();
    quads.setGroupVisible(GROUP_UNLOCK, true);
    quads.setMeshVisible(mesh, true);
    quads.setMeshBounds(mesh, static_cast<float>(m_thumbX), static_cast<float>(m_thumbY),
                        static_cast<float>(m_thumbWidth), static_cast<float>(m_thumbHeight), MESH_ANCHOR);
    quads.render(g);
    quads.setMeshVisible(mesh, false);
    quads.setGroupVisible(GROUP_UNLOCK, false);
}

void UnlockableItem::saveToLibrary() const {
    AppEngine* canvas = AppEngine::getCanvas();
    midp::ReferenceCountedPointer<midp::Image> image(canvas->getResourceManager().loadImage(m_imageId));
    canvas->getPhotoPicker()->savePhoto(image);
}

bool UnlockableItem::isUnlocked() const {
    int badges = AppEngine::getAchievementData()->getBadgeCount();
    return m_badges <= badges;
}

midp::ReferenceCountedPointer<midp::String> UnlockableItem::getDescription() const {
    if (isUnlocked()) {
        return m_unlocked;
    }
    return m_locked;
}

int UnlockableItem::getQuadId() const {
    if (isUnlocked()) {
        return m_quadId;
    }
    return MESH_LOCKED;
}

UnlockableItem::UnlockableItem(int quadId, int imageId, int textId, int badges)
    : m_quadId(quadId),
      m_imageId(imageId),
      m_unlocked(NULL),
      m_locked(NULL),
      m_badges(badges),
      m_thumbWidth(0),
      m_thumbHeight(0),
      m_thumbX(0),
      m_thumbY(0) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    m_unlocked = text->getString(textId);
    midp::ReferenceCountedPointer<midp::String> count(midp::String::valueOf(badges));
    int format = badges == 1 ? TEXT_ONE : TEXT_MANY;
    text->dynamicString(Achievement::DYNAMIC_TEXT_ID, format, count);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, Achievement::DYNAMIC_TEXT_ID);
    m_locked = buffer->toString();

    int meshW = quads.getMeshWidth(m_quadId);
    int meshH = quads.getMeshHeight(m_quadId);
    float width = static_cast<float>(meshW);
    float height = static_cast<float>(meshH);
    float thumbW;
    float thumbH;
    if (meshH < meshW) {
        float scale = FIT_WIDE / width;
        thumbW = width * scale;
        thumbH = height * scale;
    } else {
        float scale = FIT_TALL / height;
        thumbW = width * scale;
        thumbH = height * scale;
    }
    m_thumbWidth = static_cast<int>(thumbW);
    m_thumbHeight = static_cast<int>(thumbH);
    setWidth(ITEM_WIDTH);
    setHeight(ITEM_HEIGHT);
}

int UnlockableItem::getThumbWidth() const {
    return m_thumbWidth;
}

int UnlockableItem::getThumbHeight() const {
    return m_thumbHeight;
}

int UnlockableItem::getThumbX() const {
    return m_thumbX;
}

int UnlockableItem::getThumbY() const {
    return m_thumbY;
}

UnlockableItem::~UnlockableItem() {}

}
