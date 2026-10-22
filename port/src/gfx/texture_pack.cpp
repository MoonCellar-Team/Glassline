#include "texture_pack.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "../platform/settings.h"
#include "gl_proc.h"
#include "m3g/Image2D.h"
#include "m3g/Loader.h"
#include "m3g/Texture2D.h"
#include "m3g/m3g.h"
#include "png.h"
#include "port/brand.h"
#include "port/files.h"
#include "port/platform.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/GLES.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/OpenGLESManager.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "sdk/Foundation.h"

namespace m3g {
extern bool g_usingGLES20;
}

namespace port {
namespace gfx {

namespace {

namespace settings = port::platform::settings;

typedef unsigned int GLenum;
typedef int GLint;
const GLenum kRGBA = 0x1908, kUnsignedByte = 0x1401, kUnpackAlignment = 0x0cf5, kMaxTextureSize = 0x0d33;

std::string keyName(uint64_t key) {
    char name[24];
    snprintf(name, sizeof name, "%016llx.png", (unsigned long long)key);
    return name;
}

bool makeDir(const std::string& dir) { return mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST; }

struct DumpStats {
    int written = 0, known = 0, skipped = 0;
};

void dumpImage(m3g::Image2D* image, const std::string& dir, DumpStats* stats) {
    const void* data = image->getMipMapData(0);
    if (!data) return;
    uint64_t key = textureHash(data, image->getMipMapDataSize(0), image->getMipMapWidth(0),
                               image->getMipMapHeight(0), unsigned(image->getFormat()));
    std::string path = dir + "/" + keyName(key);
    if (files::stat(path).exists) {
        stats->known++;
        return;
    }
    std::vector<uint8_t> rgba;
    const char* why = "";
    if (!imageRGBA(image, &rgba, &why)) {
        fprintf(stderr, "medecomp: текстура %s (формат %d, %dx%d): %s\n", keyName(key).c_str(), image->getFormat(),
                image->getMipMapWidth(0), image->getMipMapHeight(0), why);
        stats->skipped++;
        return;
    }
    if (writePNG(path, image->getMipMapWidth(0), image->getMipMapHeight(0), rgba.data())) stats->written++;
    else fprintf(stderr, "medecomp: не записать %s\n", path.c_str());
}

void walk(m3g::Object3D* root, std::set<m3g::Object3D*>* seen, const std::string& dir, DumpStats* stats) {
    std::vector<m3g::Object3D*> stack = {root};
    while (!stack.empty()) {
        m3g::Object3D* o = stack.back();
        stack.pop_back();
        if (!o || !seen->insert(o).second) continue;
        if (m3g::Image2D* image = m3g::m3g_cast<m3g::Image2D>(o)) dumpImage(image, dir, stats);
        if (o->getM3GUniqueClassID() == 17) stack.push_back(static_cast<m3g::Texture2D*>(o)->getImage());
        midp::array<m3g::Object3D*> none;
        int count = o->getReferences(none);
        if (count <= 0) continue;
        midp::array<m3g::Object3D*> refs(count);
        o->getReferences(refs);
        for (int i = 0; i < count; i++) stack.push_back(refs[i]);
    }
}

}

bool imageRGBA(m3g::Image2D* image, std::vector<uint8_t>* out, const char** why) {
    if (!image->m_data || image->m_dataLevels <= 0) {
        *why = "данные уже отпущены (картинка в видеопамяти)";
        return false;
    }
    int format = image->getFormat() & 255;
    int w = image->getMipMapWidth(0), h = image->getMipMapHeight(0);
    const uint8_t* src = static_cast<const uint8_t*>(image->getMipMapData(0));
    int size = image->getMipMapDataSize(0);
    size_t n = size_t(w) * h;
    if (!src || w <= 0 || h <= 0) {
        *why = "нет данных";
        return false;
    }
    if (format >= 120 && format <= 125) {
        static const unsigned gl[] = {0x8c92, 0x8c93, 0x8c01, 0x8c03, 0x8c00, 0x8c02};
        if (decodePVRTCTexture(gl[format - 120], w, h, src, size, out)) return true;
        *why = format < 122 ? "ATC — не распаковать" : "PVRTC — данных мало";
        return false;
    }
    int bytes = format == 99 ? 3 : format == 100 ? 4 : format >= 101 && format <= 103 ? 2 : 0;
    if (!bytes) {
        *why = "формат не знаю";
        return false;
    }
    if (size_t(size) < n * bytes) {
        *why = "данных мало";
        return false;
    }
    out->resize(n * 4);
    for (size_t i = 0; i < n; i++) {
        uint8_t* d = &(*out)[i * 4];
        const uint8_t* s = src + i * bytes;
        unsigned v = bytes == 2 ? unsigned(s[0]) | unsigned(s[1]) << 8 : 0;
        if (format == 99) {
            d[0] = s[0], d[1] = s[1], d[2] = s[2], d[3] = 255;
        } else if (format == 100) {
            d[0] = s[0], d[1] = s[1], d[2] = s[2], d[3] = s[3];
        } else if (format == 101) {
            d[0] = uint8_t((v >> 11) * 255 / 31), d[1] = uint8_t((v >> 5 & 63) * 255 / 63);
            d[2] = uint8_t((v & 31) * 255 / 31), d[3] = 255;
        } else if (format == 102) {
            d[0] = uint8_t((v >> 11) * 255 / 31), d[1] = uint8_t((v >> 6 & 31) * 255 / 31);
            d[2] = uint8_t((v >> 1 & 31) * 255 / 31), d[3] = v & 1 ? 255 : 0;
        } else {
            d[0] = uint8_t((v >> 12) * 17), d[1] = uint8_t((v >> 8 & 15) * 17);
            d[2] = uint8_t((v >> 4 & 15) * 17), d[3] = uint8_t((v & 15) * 17);
        }
    }
    return true;
}

std::string texturesDir() { return files::dataDir() + "/textures"; }

bool texturePackSetting() { return settings::get("texture_pack", "on") != "off"; }

void setTexturePackSetting(bool on) { settings::set("texture_pack", on ? "on" : "off"); }

int dumpTextures() {
    if (!port::platform::createWindow(MEDECOMP_PORT_NAME " — textures")) return 1;
    m3g::g_usingGLES20 = true;
    midp::OpenGLESManager::setGLES(reinterpret_cast<midp::GLES*>(midp::GLES::InstanceCachedGLES20()));
    std::string dir = texturesDir();
    if (!makeDir(dir) || !makeDir(dir + "/dump")) {
        fprintf(stderr, "medecomp: не создать %s/dump\n", dir.c_str());
        return 1;
    }
    dir += "/dump";
    std::vector<std::string> names;
    for (const std::string& name : files::list(files::bundle() + "/res"))
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".m3g") == 0) names.push_back(name);
    std::sort(names.begin(), names.end());
    DumpStats stats;
    for (size_t i = 0; i < names.size(); i++) {
        NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
        midp::InputStream* in = midp::Class::getResourceAsStream(new midp::String(names[i].c_str()));
        if (!in) {
            fprintf(stderr, "medecomp: не открыть %s\n", names[i].c_str());
            pool->release();
            continue;
        }
        midp::array<m3g::Object3D*> roots = m3g::Loader::load(in);
        std::set<m3g::Object3D*> seen;
        for (int r = 0; r < roots.m_length; r++) walk(roots[r], &seen, dir, &stats);
        pool->release();
        printf("\r%zu/%zu %s\033[K", i + 1, names.size(), names[i].c_str());
        fflush(stdout);
    }
    printf("\nтекстур записано: %d, повторов: %d, пропущено: %d -> %s\n", stats.written, stats.known, stats.skipped,
           dir.c_str());
    return 0;
}

}
}

extern "C" int medecomp_enlarge_sign(const void* data, int size, int width, int height, int format, unsigned target,
                                    int minFilter, int wrapS, int wrapT) {
    using namespace port::gfx;
    if (port::platform::originalGraphics() || !data || width < 32 || height < 32) return 0;
    if (minFilter < 0x2700 || minFilter > 0x2703) return 0;
    int side = width > height ? width : height;
    int m3g = format & 255;
    std::vector<uint8_t> src;
    if (m3g >= 122 && m3g <= 125 && side <= 512) {
        static const unsigned glFormat[] = {0x8c01, 0x8c03, 0x8c00, 0x8c02};
        if (!decodePVRTCTexture(glFormat[m3g - 122], width, height, data, size, &src)) return 0;
    } else if ((m3g == 99 || m3g == 100) && side <= 1024 && (width < height ? width : height) <= 256) {
        int bpp = m3g == 99 ? 3 : 4;
        if (size < width * height * bpp) return 0;
        const uint8_t* raw = static_cast<const uint8_t*>(data);
        src.resize(size_t(width) * height * 4);
        for (int i = 0; i < width * height; i++) {
            src[i * 4] = raw[i * bpp];
            src[i * 4 + 1] = raw[i * bpp + 1];
            src[i * 4 + 2] = raw[i * bpp + 2];
            src[i * 4 + 3] = bpp == 4 ? raw[i * bpp + 3] : 255;
        }
    } else
        return 0;
    static auto getInteger = reinterpret_cast<void (*)(GLenum, GLint*)>(glProc("glGetIntegerv"));
    GLint maxTex = 0;
    getInteger(kMaxTextureSize, &maxTex);
    if (width * 2 > maxTex || height * 2 > maxTex) return 0;
    int repS = wrapS == 0x2901 || wrapS == 0x8370;
    int repT = wrapT == 0x2901 || wrapT == 0x8370;
    int ow = width * 2, oh = height * 2;
    std::vector<uint8_t> wide(size_t(ow) * height * 4), dst(size_t(ow) * oh * 4);
#define ME_EDGE(v, n, repeat) ((repeat) ? (((v) % (n) + (n)) % (n)) : ((v) < 0 ? 0 : (v) >= (n) ? (n)-1 : (v)))
    for (int y = 0; y < height; y++) {
        const uint8_t* row = &src[size_t(y) * width * 4];
        uint8_t* out = &wide[size_t(y) * ow * 4];
        for (int x = 0; x < ow; x++) {
            int a = x >> 1, b = ME_EDGE(a + ((x & 1) ? 1 : -1), width, repS);
            const uint8_t* pa = row + a * 4, *pb = row + b * 4;
            for (int c = 0; c < 4; c++) out[x * 4 + c] = (uint8_t)((pa[c] * 3 + pb[c] + 2) >> 2);
        }
    }
    for (int y = 0; y < oh; y++) {
        int a = y >> 1, b = ME_EDGE(a + ((y & 1) ? 1 : -1), height, repT);
        const uint8_t* ra = &wide[size_t(a) * ow * 4];
        const uint8_t* rb = &wide[size_t(b) * ow * 4];
        uint8_t* out = &dst[size_t(y) * ow * 4];
        for (int i = 0; i < ow * 4; i++) out[i] = (uint8_t)((ra[i] * 3 + rb[i] + 2) >> 2);
    }
    std::vector<uint8_t> sharp;
    const uint8_t* upload = dst.data();
    if (ow <= 512 && oh <= 512) {
        sharp = dst;
        for (int y = 0; y < oh; y++) {
            int ym = ME_EDGE(y - 1, oh, repT), yp = ME_EDGE(y + 1, oh, repT);
            for (int x = 0; x < ow; x++) {
                int xm = ME_EDGE(x - 1, ow, repS), xp = ME_EDGE(x + 1, ow, repS);
                const uint8_t* c0 = &dst[(size_t(y) * ow + x) * 4];
                const uint8_t* l = &dst[(size_t(y) * ow + xm) * 4];
                const uint8_t* r = &dst[(size_t(y) * ow + xp) * 4];
                const uint8_t* u = &dst[(size_t(ym) * ow + x) * 4];
                const uint8_t* d = &dst[(size_t(yp) * ow + x) * 4];
                uint8_t* o = &sharp[(size_t(y) * ow + x) * 4];
                for (int c = 0; c < 3; c++) {
                    int q = c0[c] + ((c0[c] * 4 - (l[c] + r[c] + u[c] + d[c])) >> 2);
                    if (q < 0) q = 0;
                    if (q > 255) q = 255;
                    o[c] = (uint8_t)q;
                }
                o[3] = c0[3];
            }
        }
        upload = sharp.data();
    }
#undef ME_EDGE
    static auto store = reinterpret_cast<void (*)(GLenum, GLint)>(glProc("glPixelStorei"));
    static auto tex = reinterpret_cast<void (*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLenum, GLenum, const void*)>(
        glProc("glTexImage2D"));
    static auto mipmaps = reinterpret_cast<void (*)(GLenum)>(glProc("glGenerateMipmap"));
    GLint alignment = 4;
    getInteger(kUnpackAlignment, &alignment);
    store(kUnpackAlignment, 4);
    tex(target, 0, GLint(kRGBA), ow, oh, 0, kRGBA, kUnsignedByte, upload);
    store(kUnpackAlignment, alignment);
    mipmaps(target);
    fprintf(stderr, "medecomp: текстура %dx%d -> %dx%d\n", width, height, ow, oh);
    fflush(stderr);
    return 1;
}

extern "C" int medecomp_sign_sharpen(void) { return port::platform::originalGraphics() ? 0 : 1; }

extern "C" int medecomp_texture_replace(const void* data, int size, int width, int height, int format, unsigned target) {
    using namespace port::gfx;
    static int enabled = -1;
    if (enabled < 0) {
        enabled = texturePackSetting() && !port::platform::originalGraphics() && port::files::stat(texturesDir()).dir;
        if (enabled) fprintf(stderr, "medecomp: свои текстуры: %s\n", texturesDir().c_str());
    }
    if (!enabled || !data) return 0;
    std::string name = keyName(textureHash(data, size, width, height, unsigned(format)));
    std::vector<unsigned char> bytes;
    if (!port::files::read(texturesDir() + "/" + name, &bytes)) return 0;
    Image image;
    static auto getInteger = reinterpret_cast<void (*)(GLenum, GLint*)>(glProc("glGetIntegerv"));
    GLint max = 0;
    getInteger(kMaxTextureSize, &max);
    if (!decodePNG(bytes.data(), bytes.size(), image, false) || image.width > max || image.height > max) {
        fprintf(stderr, "medecomp: своя текстура %s не подошла (не PNG или больше %d)\n", name.c_str(), int(max));
        return 0;
    }
    static auto store = reinterpret_cast<void (*)(GLenum, GLint)>(glProc("glPixelStorei"));
    static auto tex = reinterpret_cast<void (*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLenum, GLenum, const void*)>(
        glProc("glTexImage2D"));
    static auto mipmaps = reinterpret_cast<void (*)(GLenum)>(glProc("glGenerateMipmap"));
    GLint alignment = 4;
    getInteger(kUnpackAlignment, &alignment);
    store(kUnpackAlignment, 4);
    tex(target, 0, GLint(kRGBA), image.width, image.height, 0, kRGBA, kUnsignedByte, image.rgba.data());
    store(kUnpackAlignment, alignment);
    mipmaps(target);
    fprintf(stderr, "medecomp: своя текстура %s: %dx%d вместо %dx%d\n", name.c_str(), image.width, image.height, width,
            height);
    return 1;
}
