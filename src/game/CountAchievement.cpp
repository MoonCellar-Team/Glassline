#include "game/CountAchievement.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

CountAchievement::CountAchievement(int idx, int nameId, int descriptionId, int count)
    : Achievement(idx, nameId, descriptionId), m_count(count) {}

midp::StringBuffer* CountAchievement::getNameStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> count = midp::String::valueOf(m_count);
    text->dynamicString(DYNAMIC_TEXT_ID, m_nameId, count);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

midp::StringBuffer* CountAchievement::getDescriptionStringBuffer() {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::ReferenceCountedPointer<midp::String> count = text->convertIntToStringWithThousandSep(m_count);
    text->dynamicString(DYNAMIC_TEXT_ID, m_descriptionId, count);
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendStringIdToBuffer(buffer, DYNAMIC_TEXT_ID);
    return buffer;
}

int CountAchievement::getCount() const {
    return m_count;
}

CountAchievement::~CountAchievement() {}
