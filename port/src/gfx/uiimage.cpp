#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <zlib.h>

#include <string>

#include "../platform/launcher.h"
#include "../platform/overlay.h"
#include "cg.h"
#include "port/brand.h"
#include "port/platform.h"
#include "port/objc.h"
#include "sdk/UIKit.h"

namespace {

struct ImageObject : port::objc::Object {
    CGImage* image = nullptr;
    ImageObject();
    ~ImageObject() {
        if (image) CFRelease(image);
    }
};
const port::objc::Class kUIImage = {
    {"UIImage", port::objc::destroyAs<ImageObject>, nullptr}, &port::objc::NSObjectClass, {}};
ImageObject::ImageObject() : port::objc::Object(&kUIImage) {}

ImageObject* obj(UIImage* p) { return reinterpret_cast<ImageObject*>(p); }

void put32(std::string& s, uint32_t v) {
    char b[4] = {char(v >> 24), char(v >> 16), char(v >> 8), char(v)};
    s.append(b, 4);
}

void chunk(std::string& out, const char* type, const std::string& body) {
    put32(out, uint32_t(body.size()));
    std::string t = std::string(type, 4) + body;
    out += t;
    put32(out, uint32_t(crc32(0, reinterpret_cast<const Bytef*>(t.data()), uInt(t.size()))));
}

bool writePNG(CGImage* image, const std::string& path) {
    int w = port::gfx::imageWidth(image), h = port::gfx::imageHeight(image);
    if (w <= 0 || h <= 0) return false;
    const uint8_t* px = port::gfx::imagePixels(image);
    std::string raw;
    raw.reserve(size_t(h) * (w * 4 + 1));
    for (int y = 0; y < h; y++) {
        raw += '\0';
        for (int x = 0; x < w; x++) {
            const uint8_t* p = px + (size_t(y) * w + x) * 4;
            int a = p[3];
            for (int c = 0; c < 3; c++) raw += char(a ? (p[c] * 255 + a / 2) / a : 0);
            raw += char(a);
        }
    }
    uLongf size = compressBound(uLong(raw.size()));
    std::string z(size, '\0');
    if (compress2(reinterpret_cast<Bytef*>(&z[0]), &size, reinterpret_cast<const Bytef*>(raw.data()), uLong(raw.size()),
                  6) != Z_OK)
        return false;
    z.resize(size);
    std::string ihdr;
    put32(ihdr, uint32_t(w));
    put32(ihdr, uint32_t(h));
    ihdr += std::string("\x08\x06\x00\x00\x00", 5);
    std::string out = "\x89PNG\r\n\x1a\n";
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", "");
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    return fclose(f) == 0 && ok;
}

std::string albumPath() {
    const char* home = getenv("HOME");
    std::string base = home ? home : ".";
    struct stat st;
    if (stat((base + "/Pictures").c_str(), &st) == 0 && S_ISDIR(st.st_mode)) base += "/Pictures";
    std::string dir = base + "/" MEDECOMP_PORT_NAME;
    mkdir(dir.c_str(), 0755);
    time_t now = time(nullptr);
    char name[64];
    strftime(name, sizeof name, "photo-%Y%m%d-%H%M%S", localtime(&now));
    std::string path = dir + "/" + name + ".png";
    for (int i = 2; stat(path.c_str(), &st) == 0; i++) path = dir + "/" + name + "-" + std::to_string(i) + ".png";
    return path;
}

}

UIImage* UIImage::imageWithCGImage(CGImageRef image) {
    ImageObject* o = new ImageObject();
    o->image = image;
    if (image) CFRetain(image);
    return reinterpret_cast<UIImage*>(o);
}

UIImage* UIImage::alloc() { return reinterpret_cast<UIImage*>(new ImageObject()); }

UIImage* UIImage::initWithData(NSData* data) {
    if (!this) return nullptr;
    if (data) fprintf(stderr, "medecomp: UIImage из NSData пока не читается\n");
    CFRelease(this);
    return nullptr;
}

UIImage* UIImage::autorelease() { return this; }

CGImageRef UIImage::CGImage() { return this ? obj(this)->image : nullptr; }

extern "C" void UIImageWriteToSavedPhotosAlbum(UIImage* image, void* target, SEL selector, void* contextInfo) {
    CGImage* cg = image ? obj(image)->image : nullptr;
    std::string path = albumPath();
    bool ok = cg && writePNG(cg, path);
#ifdef __ANDROID__
    if (ok) {
        std::string shown = port::platform::androidSaveToDownloads(path, path.substr(path.find_last_of('/') + 1), "image/png");
        if (!shown.empty()) path = shown;
    }
#endif
    if (ok)
        fprintf(stderr, "medecomp: фото сохранено: %s\n", path.c_str());
    else
        fprintf(stderr, "medecomp: фото не сохранено: %s\n", path.c_str());
    const bool ru = port::platform::launcherRussian();
    port::platform::notify(ok ? (ru ? "Обои сохранены" : "Wallpaper saved") : (ru ? "Обои не сохранены" : "Wallpaper not saved"), path);
    if (!target || !selector) return;
    const char* sel = reinterpret_cast<const char*>(selector);
    if (image) CFRetain(image);
    port::objc::onMainThread([=] {
        static const void* error = port::cf::makeString(std::string("не сохранено"));
        port::objc::send(target, sel, image, ok ? nullptr : const_cast<void*>(error), contextInfo);
        if (image) CFRelease(image);
    }, false);
}
