#include "runtime/midp/Image.h"

#include "runtime/midp/ImageClass.h"

namespace midp {

Class* Image::getClass() const {
    return new ImageClass();
}

void Image::setSize(int width, int height) {
    m_width = width;
    m_height = height;
}

int Image::getWidth() const {
    return m_width;
}

int Image::getHeight() const {
    return m_height;
}

GraphicsIPhoneCG* Image::getGraphics() {
    return implementation_Image_getGraphics(this);
}

ImageIPhone* Image::createImage(int width, int height) {
    return implementation_Image_createImage(width, height);
}

ImageIPhone* Image::createImage(InputStream* stream) {
    return implementation_Image_createImage(stream);
}

Image::~Image() {}

Image::Image(bool mutableImage) : m_width(0), m_height(0), m_mutable(mutableImage) {}

}
