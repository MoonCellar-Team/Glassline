#include "game/Achievement.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"

Achievement::Achievement(int idx, int nameId, int descriptionId)
    : m_idx(idx), m_nameId(nameId), m_descriptionId(descriptionId), m_complete(false) {}

Achievement::~Achievement() {}

midp::StringBuffer* Achievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, m_nameId);
    return buffer;
}

midp::StringBuffer* Achievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, m_descriptionId);
    return buffer;
}

int Achievement::getIdx() const {
    return m_idx;
}

int Achievement::getNameId() const {
    return m_nameId;
}

bool Achievement::isComplete() {
    return m_complete;
}

void Achievement::complete() {
    m_complete = true;
}
