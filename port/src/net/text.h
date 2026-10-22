#pragma once

#include <string.h>

#include <string>

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

namespace port {
namespace net {

inline std::string utf8(midp::String* s) {
    if (!s) return std::string();
    midp::array<signed char> bytes = s->getBytes("UTF-8");
    return std::string(reinterpret_cast<const char*>(bytes.data()), size_t(bytes.m_length));
}

inline midp::String* fromUtf8(const std::string& text) {
    midp::array<signed char> bytes(int(text.size()));
    for (size_t i = 0; i < text.size(); i++) bytes[int(i)] = static_cast<signed char>(text[i]);
    return new midp::String(bytes, "UTF-8");
}

inline std::string gameText(int id) { return utf8(AppEngine::getCanvas()->getTextManager()->getString(id)); }

inline std::string replaced(std::string text, const char* token, const std::string& value) {
    size_t at = text.find(token);
    if (at != std::string::npos) text.replace(at, strlen(token), value);
    return text;
}

inline std::string millisText(int millis) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::StringBuffer* buffer = text->clearStringBuffer();
    text->appendMillisTimeToBuffer(buffer, millis, 2);
    midp::ReferenceCountedPointer<midp::String> s(buffer->toString());
    return utf8(s);
}

}
}
