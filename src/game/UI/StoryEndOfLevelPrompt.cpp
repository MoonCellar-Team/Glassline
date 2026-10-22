#include "game/UI/StoryEndOfLevelPrompt.h"

#include "game/Achievement.h"
#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int TEXT_BAGS = 0x8fe;
static const int FONT_BAGS = 0x1e;
static const int GROUP_BAGS = 0x36;
static const int MESH_BAGS = 0x80;
static const int MESH_ANCHOR = 10;
static const int TEXT_ANCHOR = 0xc;
static const int RENDER_SCOPE = 2;
static const float MESH_Y = 31.0f;

namespace UI {

void StoryEndOfLevelPrompt::render(midp::Graphics* g, int originY, int originX) {
    EndOfLevelPrompt::render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int meshWidth = m_quadManager->getMeshWidth(MESH_BAGS);
    int x = (m_width - 0x1e) - (meshWidth >> 1);
    m_quadManager->setGroupVisible(GROUP_BAGS, true);
    m_quadManager->setMeshVisible(MESH_BAGS, true);
    m_quadManager->setMeshPosition(MESH_BAGS, static_cast<float>(x), MESH_Y, MESH_ANCHOR);
    m_quadManager->render(g, RENDER_SCOPE);
    m_quadManager->setMeshVisible(MESH_BAGS, false);
    m_quadManager->setGroupVisible(GROUP_BAGS, false);
    text->drawString(g, m_bags, FONT_BAGS, x - 0x28, 0x4c, TEXT_ANCHOR);
}

StoryEndOfLevelPrompt::StoryEndOfLevelPrompt()
    : m_bags(NULL) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    midp::ReferenceCountedPointer<midp::String> found(midp::String::valueOf(level->getNumBagsFound()));
    midp::ReferenceCountedPointer<midp::String> total(midp::String::valueOf(level->getNumTotalBags()));
    text->dynamicString(Achievement::DYNAMIC_TEXT_ID, TEXT_BAGS, found, total);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, Achievement::DYNAMIC_TEXT_ID);
    m_bags = buffer->toString();
}

void StoryEndOfLevelPrompt::UnHide() {}

StoryEndOfLevelPrompt::~StoryEndOfLevelPrompt() {}

}
