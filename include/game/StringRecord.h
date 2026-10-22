#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Image;
class String;
}

class StringRecord {
public:
    StringRecord();
    ~StringRecord();
    void set(midp::String* text, midp::Image* image);
    midp::Image* getImage();
    bool equals(midp::String* text);

    midp::ReferenceCountedPointer<midp::String> m_text;
    midp::ReferenceCountedPointer<midp::Image> m_image;
};
