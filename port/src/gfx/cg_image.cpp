#include <stdio.h>
#include <string.h>

#include "cg.h"
#include "png.h"

namespace {

using port::cf::Object;

struct Provider : Object {
    void* info;
    CGDataProviderSequentialCallbacks callbacks;
    Provider(void* i, const CGDataProviderSequentialCallbacks& cb);
    ~Provider() {
        if (callbacks.releaseInfo) callbacks.releaseInfo(info);
    }
};
void destroyProvider(Object* o) { delete static_cast<Provider*>(o); }
const port::cf::Class kProvider = {"CGDataProvider", destroyProvider, nullptr};
Provider::Provider(void* i, const CGDataProviderSequentialCallbacks& cb) : Object(&kProvider), info(i), callbacks(cb) {}

const port::cf::Class kColorSpace = {"CGColorSpace", [](Object* o) { delete o; }, nullptr};

struct Color : Object {
    float rgba[4];
    Color();
};
const port::cf::Class kColor = {"CGColor", [](Object* o) { delete static_cast<Color*>(o); }, nullptr};
Color::Color() : Object(&kColor), rgba{0, 0, 0, 1} {}

}

struct CGImage : Object {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;
    cairo_surface_t* surface = nullptr;
    CGImage();
    ~CGImage() {
        if (surface) cairo_surface_destroy(surface);
    }
};
namespace {
const port::cf::Class kImage = {"CGImage", [](Object* o) { delete static_cast<CGImage*>(o); }, nullptr};
}
CGImage::CGImage() : Object(&kImage) {}

namespace port {
namespace gfx {

std::vector<uint8_t> providerBytes(void* p) {
    std::vector<uint8_t> out;
    Provider* provider = static_cast<Provider*>(p);
    if (!provider || !provider->callbacks.getBytes) return out;
    uint8_t buf[65536];
    for (;;) {
        unsigned long n = provider->callbacks.getBytes(provider->info, buf, sizeof buf);
        if (n == 0 || n > sizeof buf) break;
        out.insert(out.end(), buf, buf + n);
    }
    return out;
}

void colorComponents(const void* color, float rgba[4]) {
    memcpy(rgba, static_cast<const Color*>(color)->rgba, sizeof(float) * 4);
}

CGImage* makeImage(int width, int height) {
    CGImage* image = new CGImage();
    image->width = width < 0 ? 0 : width;
    image->height = height < 0 ? 0 : height;
    image->rgba.assign(size_t(image->width) * image->height * 4, 0);
    return image;
}

uint8_t* imagePixels(CGImage* image) { return image->rgba.data(); }
int imageWidth(CGImage* image) { return image ? image->width : 0; }
int imageHeight(CGImage* image) { return image ? image->height : 0; }

cairo_surface_t* imageSurface(CGImage* image) {
    if (!image->surface && image->width > 0 && image->height > 0)
        image->surface = cairo_image_surface_create_for_data(image->rgba.data(), CAIRO_FORMAT_ARGB32, image->width,
                                                             image->height, image->width * 4);
    return image->surface;
}

}
}

extern "C" {

void* CGDataProviderCreateSequential(void* info, const CGDataProviderSequentialCallbacks* callbacks) {
    return new Provider(info, *callbacks);
}

void CGDataProviderRelease(void* provider) { CFRelease(provider); }

void* CGColorSpaceCreateDeviceRGB() { return new Object(&kColorSpace); }

void CGColorSpaceRelease(void* space) { CFRelease(space); }

void* CGColorCreate(void*, const float* components) {
    Color* c = new Color();
    if (components) memcpy(c->rgba, components, sizeof c->rgba);
    return c;
}

void CGColorRelease(void* color) { CFRelease(color); }

CGImageRef CGImageCreateWithPNGDataProvider(void* provider, const float*, bool, int) {
    std::vector<uint8_t> bytes = port::gfx::providerBytes(provider);
    port::gfx::Image png;
    if (!port::gfx::decodePNG(bytes.data(), bytes.size(), png)) {
        fprintf(stderr, "medecomp: PNG не читается (%zu байт)\n", bytes.size());
        return nullptr;
    }
    CGImage* image = new CGImage();
    image->width = png.width;
    image->height = png.height;
    image->rgba = std::move(png.rgba);
    return image;
}

CGImageRef CGImageCreateWithImageInRect(CGImageRef image, CGRect rect) {
    if (!image) return nullptr;
    int x0 = int(rect.origin.x), y0 = int(rect.origin.y);
    int x1 = int(rect.origin.x + rect.size.width), y1 = int(rect.origin.y + rect.size.height);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > image->width) x1 = image->width;
    if (y1 > image->height) y1 = image->height;
    if (x1 <= x0 || y1 <= y0) return nullptr;
    CGImage* part = port::gfx::makeImage(x1 - x0, y1 - y0);
    for (int y = y0; y < y1; y++)
        memcpy(&part->rgba[size_t(y - y0) * part->width * 4], &image->rgba[(size_t(y) * image->width + x0) * 4],
               size_t(part->width) * 4);
    return part;
}

CGImageRef CGImageRetain(CGImageRef image) {
    CFRetain(image);
    return image;
}

void CGImageRelease(CGImageRef image) { CFRelease(image); }

unsigned long CGImageGetWidth(CGImageRef image) { return image ? (unsigned long)image->width : 0; }

unsigned long CGImageGetHeight(CGImageRef image) { return image ? (unsigned long)image->height : 0; }

}
