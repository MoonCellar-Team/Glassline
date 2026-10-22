#pragma once

#include "runtime/midp/InputStream.h"
#include "sdk/CoreGraphics.h"
#include "sdk/pthread.h"

namespace midp {

class Class;
class GraphicsIPhoneCG;
class ImageIPhone;
class String;

class Image : public Object {
public:
    explicit Image(bool mutableImage);
    virtual ~Image();
    virtual Class* getClass() const;
    virtual void getRGB(array<int>& rgb, int offset, int scanlength, int x, int y, int width,
                        int height) const = 0;
    virtual void getRGB(int* rgb, int offset, int scanlength, int x, int y, int width,
                        int height) const = 0;
    void setSize(int width, int height);
    int getWidth() const;
    int getHeight() const;
    GraphicsIPhoneCG* getGraphics();
    static ImageIPhone* createImage(int width, int height);
    static ImageIPhone* createImage(InputStream* stream);

    int m_width;
    int m_height;
    bool m_mutable;
};

class ImageIPhone : public Image {
public:
    ImageIPhone(int width, int height);
    ImageIPhone(InputStream* stream);
    void invalidate();
    explicit ImageIPhone(void* cgImage);
    CGImageRef getCGImageRef() const;

    CGContext* m_context;
    mutable CGImageRef m_cgImage;
    unsigned char* m_pixels;
    mutable unsigned int m_texture;
    mutable int m_textureOffsetX;
    mutable int m_textureOffsetY;
    mutable int m_textureWidth;
    mutable int m_textureHeight;
    mutable bool m_invalid;
#ifdef MEDECOMP_PORT
    unsigned char m_scale = 1;
#endif

    ImageIPhone();
    virtual ~ImageIPhone();
    bool isInvalid() const;
    int getTextureOffsetX() const;
    int getTextureOffsetY() const;
    int getTextureWidth() const;
    int getTextureHeight() const;
    virtual void getRGB(array<int>& rgb, int offset, int scanlength, int x, int y, int width,
                        int height) const;
    virtual void getRGB(int* rgb, int offset, int scanlength, int x, int y, int width,
                        int height) const;
    void upload(int x, int y, int width, int height) const;
    void bindTexture() const;
    void dispose();
    void set(int width, int height, int bytesPerPixel);
    void setFrom(void* cgImage);
};

#ifdef MEDECOMP_PORT
ImageIPhone* createScaledImage(int width, int height, int scale);
#endif

class WebImage : public ImageIPhone {
public:
    enum WebImageResult {
        kPending = 0,
        kSuccess = 1,
    };

    explicit WebImage(String* url);
    virtual ~WebImage();
    virtual void run();
    WebImageResult GetResult();
    void SetResult(WebImageResult result);

    String* m_url;
    pthread_t m_thread;
    int m_result;
    pthread_mutex_t m_mutex;
};

ImageIPhone* implementation_Image_createImage(int width, int height);
ImageIPhone* implementation_Image_createImage(InputStream* stream);
GraphicsIPhoneCG* implementation_Image_getGraphics(Image* image);

}
