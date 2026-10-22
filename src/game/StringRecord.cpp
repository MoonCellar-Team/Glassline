#include "game/StringRecord.h"

#include <stddef.h>

#include "runtime/midp/Image.h"
#include "runtime/midp/String.h"

StringRecord::StringRecord() : m_text(NULL), m_image(NULL) {}

StringRecord::~StringRecord() {}

void StringRecord::set(midp::String* text, midp::Image* image) {
    m_text = text;
    m_image = image;
}

midp::Image* StringRecord::getImage() {
    return m_image;
}

bool StringRecord::equals(midp::String* text) {
    if (m_text != NULL) {
        return m_text->equals(text);
    }
    return false;
}
