#include "runtime/midp/Image.h"

#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/GLES.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/array.h"
#include "sdk/CoreGraphics.h"
#include "sdk/OpenGLES/ES2/gl.h"
#include "sdk/string.h"

void* operator new[](size_t);
void operator delete[](void*);

namespace m3g {
void glesRemoveTexture(int texture);
void gles20RemoveTexture(int texture);
}

namespace {

const int kUnpackAlignment = 0x0CF5;
const int kRgba = 0x1908;
const int kUnsignedByte = 0x1401;

CGRect CGRectMake(float width, float height, float, float) {
    CGRect rect;
    rect.origin.x = 0.0f;
    rect.origin.y = 0.0f;
    rect.size.width = width;
    rect.size.height = height;
    return rect;
}

int textureDimension(int value) {
    int result = value;
    for (int n = 0; n < 12; ++n) {
        const int pot = 1 << n;
        if (pot >= value) {
            result = pot;
            break;
        }
    }
    return result;
}

#ifdef MEDECOMP_PORT
void padTextureEdges(const unsigned char* pixels, int width, int height) {
    if (pixels == NULL || width <= 0 || height <= 0)
        return;
    const int texWidth = textureDimension(width);
    const int texHeight = textureDimension(height);
    midp::GLES* gl = static_cast<midp::GLES*>(midp::DisplayIPhone::getStaticDisplay()->getGL());
    if (texWidth > width) {
        const int rows = texHeight > height ? height + 1 : height;
        unsigned char* column = new unsigned char[rows * 4];
        for (int row = 0; row < rows; ++row) {
            const int src = row < height ? row : height - 1;
            memcpy(column + row * 4, pixels + (src * width + width - 1) * 4, 4);
        }
        gl->glTexSubImage2D(GL_TEXTURE_2D, 0, width, 0, 1, rows, kRgba, kUnsignedByte, column);
        delete[] column;
    }
    if (texHeight > height)
        gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, height, width, 1, kRgba, kUnsignedByte,
                            pixels + (height - 1) * width * 4);
}
#endif

}

namespace midp {

ImageIPhone::ImageIPhone()
    : Image(false),
      m_context(NULL),
      m_cgImage(NULL),
      m_pixels(NULL),
      m_texture(0),
      m_textureOffsetX(0),
      m_textureOffsetY(0),
      m_textureWidth(0),
      m_textureHeight(0),
      m_invalid(false) {}

bool ImageIPhone::isInvalid() const {
    return m_invalid;
}

int ImageIPhone::getTextureOffsetX() const {
    return m_textureOffsetX;
}

int ImageIPhone::getTextureOffsetY() const {
    return m_textureOffsetY;
}

int ImageIPhone::getTextureWidth() const {
    return m_textureWidth;
}

int ImageIPhone::getTextureHeight() const {
    return m_textureHeight;
}

void ImageIPhone::getRGB(array<int>& rgb, int offset, int scanlength, int x, int y, int width,
                         int height) const {
    getRGB(&rgb[0], offset, scanlength, x, y, width, height);
}

CGImageRef ImageIPhone::getCGImageRef() const {
    if (m_cgImage == NULL)
        m_cgImage = CGBitmapContextCreateImage(m_context);
    return m_cgImage;
}

void ImageIPhone::getRGB(int* rgb, int offset, int scanlength, int x, int y, int width,
                         int height) const {
    if (height < 1)
        return;
    const int imageWidth = getWidth();
    int* dst = rgb + offset;
    int src = (imageWidth * y + x) * 4;
    for (int row = 0; row < height; ++row) {
        if (width >= 1) {
            const unsigned char* pixel = m_pixels + src;
            for (int col = 0; col < width; ++col) {
                dst[col] = pixel[2] | (pixel[0] << 16) | (pixel[1] << 8) | (pixel[3] << 24);
                pixel += 4;
            }
        }
        src += imageWidth * 4;
        dst += scanlength;
    }
}

void ImageIPhone::upload(int x, int y, int width, int height) const {
    const unsigned int oldTexture = m_texture;
    if (m_texture == 0) {
        GLES* gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
        gl->glGenTextures(1, &m_texture);
    }
    GLES* gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
#ifdef MEDECOMP_PORT
    const int scale = m_scale;
    const int imageWidth = getWidth() * scale;
    const int imageHeight = getHeight() * scale;
    if (scale != 1) {
        x = 0;
        y = 0;
        width = imageWidth;
        height = imageHeight;
    }
#else
    const int imageWidth = getWidth();
    const int imageHeight = getHeight();
#endif
    int texWidth = imageWidth;
    int texHeight = imageHeight;
    if (oldTexture == 0) {
        texWidth = textureDimension(imageWidth);
        texHeight = textureDimension(imageHeight);
        gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
        gl->glTexImage2D(GL_TEXTURE_2D, 0, kRgba, texWidth, texHeight, 0, kRgba, kUnsignedByte,
                         NULL);
    }
    gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
    gl->glPixelStorei(kUnpackAlignment, 1);
    if (imageHeight == height || imageWidth == width ||
        ((width * height) << 2) >= 131072) {
        gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
        gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, kRgba, kUnsignedByte,
                            m_pixels);
    } else {
        static unsigned char tempImageData[131072];
        if (height >= 1) {
            unsigned char* dst = tempImageData;
            int src = x * 4 + imageWidth * (imageHeight - (height + y)) * 4;
            int rows = height;
            do {
                --rows;
                memcpy(dst, m_pixels + src, static_cast<size_t>(width * 4));
                src += imageWidth * 4;
                dst += width * 4;
            } while (rows != 0);
        }
        gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
        gl->glTexSubImage2D(GL_TEXTURE_2D, 0, x, imageHeight - (height + y), width, height, kRgba,
                            kUnsignedByte, tempImageData);
    }
#ifdef MEDECOMP_PORT
    padTextureEdges(m_pixels, imageWidth, imageHeight);
    m_textureOffsetX = 0;
    m_textureOffsetY = imageHeight / scale;
    m_textureWidth = texWidth / scale;
    m_textureHeight = -texHeight / scale;
#else
    m_textureOffsetX = 0;
    m_textureOffsetY = imageHeight;
    m_textureWidth = texWidth;
    m_textureHeight = -texHeight;
#endif
}

void ImageIPhone::bindTexture() const {
    if (m_invalid) {
        upload(0, 0, getWidth(), getHeight());
        m_invalid = false;
    }
    GLES* gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
}

void ImageIPhone::invalidate() {
    if (m_cgImage != NULL) {
        CGImageRef image = m_cgImage;
        m_cgImage = NULL;
        CGImageRelease(image);
    }
    m_invalid = true;
}

void ImageIPhone::dispose() {
    invalidate();
    if (m_context != NULL) {
        CGContext* context = m_context;
        m_context = NULL;
        CGContextRelease(context);
    }
    if (m_pixels == NULL)
        return;
    delete[] m_pixels;
    m_pixels = NULL;
}

void ImageIPhone::set(int width, int height, int bytesPerPixel) {
    setSize(width, height);
    dispose();
#ifdef MEDECOMP_PORT
    const int scale = m_scale;
    const int stride = bytesPerPixel * width * scale;
    const int byteCount = stride * height * scale;
    m_pixels = new unsigned char[byteCount];
    memset(m_pixels, 0, static_cast<size_t>(byteCount));
    void* colorSpace = CGColorSpaceCreateDeviceRGB();
    m_context = CGBitmapContextCreate(m_pixels, static_cast<unsigned long>(width * scale),
                                      static_cast<unsigned long>(height * scale), 8,
                                      static_cast<unsigned long>(stride), colorSpace, 1);
    if (scale != 1 && m_context != NULL)
        medecomp_cg_base_scale(m_context, static_cast<float>(scale));
#else
    const int stride = bytesPerPixel * width;
    const int byteCount = stride * height;
    m_pixels = new unsigned char[byteCount];
    memset(m_pixels, 0, static_cast<size_t>(byteCount));
    void* colorSpace = CGColorSpaceCreateDeviceRGB();
    m_context = CGBitmapContextCreate(m_pixels, static_cast<unsigned long>(width),
                                      static_cast<unsigned long>(height), 8,
                                      static_cast<unsigned long>(stride), colorSpace, 1);
#endif
}

void ImageIPhone::setFrom(void* cgImage) {
    const int width = static_cast<int>(CGImageGetWidth(static_cast<CGImageRef>(cgImage)));
    const int height = static_cast<int>(CGImageGetHeight(static_cast<CGImageRef>(cgImage)));
    set(width, height, 4);
    CGContext* context = m_context;
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, 0.0f, static_cast<float>(height));
    CGContextScaleCTM(context, 1.0f, -1.0f);
    const CGRect rect = CGRectMake(static_cast<float>(width), static_cast<float>(height), 0.0f, 0.0f);
    CGContextDrawImage(context, rect, static_cast<CGImageRef>(cgImage));
    CGContextRestoreGState(context);
    invalidate();
}

ImageIPhone::~ImageIPhone() {
    dispose();
    if (m_texture != 0) {
        GLES* gl = static_cast<GLES*>(DisplayIPhone::getStaticDisplay()->getGL());
        if (gl->getGLESVersion() == 2)
            m3g::gles20RemoveTexture(static_cast<int>(m_texture));
        else
            m3g::glesRemoveTexture(static_cast<int>(m_texture));
        m_texture = 0;
    }
}

ImageIPhone::ImageIPhone(int width, int height)
    : Image(true),
      m_context(NULL),
      m_cgImage(NULL),
      m_pixels(NULL),
      m_texture(0),
      m_textureOffsetX(0),
      m_textureOffsetY(0),
      m_textureWidth(0),
      m_textureHeight(0),
      m_invalid(false) {
    set(width, height, 4);
}

ImageIPhone::ImageIPhone(void* cgImage)
    : Image(false),
      m_context(NULL),
      m_cgImage(NULL),
      m_pixels(NULL),
      m_texture(0),
      m_textureOffsetX(0),
      m_textureOffsetY(0),
      m_textureWidth(0),
      m_textureHeight(0),
      m_invalid(false) {
    setFrom(cgImage);
}

ImageIPhone::ImageIPhone(InputStream* stream)
    : Image(false),
      m_context(NULL),
      m_cgImage(NULL),
      m_pixels(NULL),
      m_texture(0),
      m_textureOffsetX(0),
      m_textureOffsetY(0),
      m_textureWidth(0),
      m_textureHeight(0),
      m_invalid(false) {
    INCREF(stream);
    CGDataProviderSequentialCallbacks callbacks;
    callbacks.version = 0;
    callbacks.getBytes = InputStreamGetBytes;
    callbacks.skipForward = InputStreamSkipBytes;
    callbacks.rewind = InputStreamRewind;
    callbacks.releaseInfo = InputStreamRelease;
    void* provider = CGDataProviderCreateSequential(stream, &callbacks);
    CGImageRef image = CGImageCreateWithPNGDataProvider(provider, NULL, false, 0);
    setFrom(image);
    CGImageRelease(image);
    CGDataProviderRelease(provider);
}

#ifdef MEDECOMP_PORT
ImageIPhone* createScaledImage(int width, int height, int scale) {
    ImageIPhone* image = new ImageIPhone(1, 1);
    image->m_scale = static_cast<unsigned char>(scale < 1 ? 1 : scale);
    image->set(width, height, 4);
    return image;
}
#endif

}
