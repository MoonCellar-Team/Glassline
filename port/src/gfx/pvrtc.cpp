#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include "gl_proc.h"
#include "port/files.h"
#include "texture_pack.h"

namespace {

typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;

const GLenum GL_RGBA = 0x1908;
const GLenum GL_UNSIGNED_BYTE = 0x1401;
const GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;
const GLenum kPVRTC_RGB_4 = 0x8C00, kPVRTC_RGB_2 = 0x8C01, kPVRTC_RGBA_4 = 0x8C02, kPVRTC_RGBA_2 = 0x8C03;

uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

struct Color {
    int r, g, b, a;
};

int expand5(int v) { return (v << 3) | (v >> 2); }
int expand4(int v) { return (v << 4) | v; }

Color colorA(uint32_t c) {
    if (c & 0x8000)
        return {expand5((c >> 10) & 31), expand5((c >> 5) & 31), expand5(((c >> 1) & 15) << 1 | ((c >> 4) & 1)), 255};
    int r = (c >> 8) & 15, g = (c >> 4) & 15, b = (c >> 1) & 7, a = (c >> 12) & 7;
    return {expand4(r), expand4(g), expand5(b << 2 | b >> 1), expand4(a << 1)};
}

Color colorB(uint32_t c) {
    if (c & 0x80000000u)
        return {expand5((c >> 26) & 31), expand5((c >> 21) & 31), expand5((c >> 16) & 31), 255};
    int r = (c >> 24) & 15, g = (c >> 20) & 15, b = (c >> 16) & 15, a = (c >> 28) & 7;
    return {expand4(r), expand4(g), expand4(b), expand4(a << 1)};
}

struct Decoder {
    const uint8_t* data;
    int bw, bh, bx, by;
    bool two;

    uint32_t index(int x, int y) const {
        uint32_t out = 0, shift = 0;
        uint32_t minDim = bx < by ? bx : by;
        for (uint32_t bit = 1; bit < minDim; bit <<= 1, shift++) {
            if (uint32_t(y) & bit) out |= 1u << (2 * shift);
            if (uint32_t(x) & bit) out |= 1u << (2 * shift + 1);
        }
        uint32_t rest = (bx > by ? uint32_t(x) : uint32_t(y)) >> shift;
        return out | rest << (2 * shift);
    }
    const uint8_t* block(int x, int y) const {
        x = ((x % bx) + bx) % bx;
        y = ((y % by) + by) % by;
        return data + size_t(index(x, y)) * 8;
    }

    int code2(const uint8_t* b, int lx, int ly) const {
        uint32_t mod = le32(b), color = le32(b + 4);
        if (!(color & 1)) return (mod >> (ly * 8 + lx)) & 1 ? 3 : 0;
        if (mod & 1) mod = (mod & ~(1u << 20)) | ((mod >> 21) & 1) << 20;
        mod = (mod & ~1u) | ((mod >> 1) & 1);
        int n = 0;
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 8; x++)
                if (((x ^ y) & 1) == 0) {
                    if (x == lx && y == ly) return (mod >> (2 * n)) & 3;
                    n++;
                }
        return 0;
    }

    int modulation(int x, int y, bool* punch) const {
        static const int rep[4] = {0, 3, 5, 8};
        int cx = x / bw, cy = y / bh, lx = x % bw, ly = y % bh;
        const uint8_t* b = block(cx, cy);
        uint32_t mod = le32(b), color = le32(b + 4);
        *punch = false;
        if (!two) {
            int v = (mod >> (2 * (ly * 4 + lx))) & 3;
            if (!(color & 1)) return rep[v];
            if (v == 2) *punch = true;
            return v == 0 ? 0 : v == 3 ? 8 : 4;
        }
        if (!(color & 1) || ((lx ^ ly) & 1) == 0) return rep[code2(b, lx, ly)];
        auto at = [&](int px, int py) {
            int qx = ((px % (bw * bx)) + bw * bx) % (bw * bx), qy = ((py % (bh * by)) + bh * by) % (bh * by);
            return rep[code2(block(qx / bw, qy / bh), qx % bw, qy % bh)];
        };
        int sub = !(mod & 1) ? 1 : (mod & (1u << 20)) ? 3 : 2;
        if (sub == 1) return (at(x, y - 1) + at(x, y + 1) + at(x - 1, y) + at(x + 1, y) + 2) / 4;
        if (sub == 2) return (at(x - 1, y) + at(x + 1, y) + 1) / 2;
        return (at(x, y - 1) + at(x, y + 1) + 1) / 2;
    }

    void decode(int width, int height, bool alpha, uint8_t* out) const {
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++) {
                int fx = x - bw / 2, fy = y - bh / 2;
                int x0 = fx >= 0 ? fx / bw : -((-fx + bw - 1) / bw), y0 = fy >= 0 ? fy / bh : -((-fy + bh - 1) / bh);
                int kx = fx - x0 * bw, ky = fy - y0 * bh;
                float wx = float(kx) / bw, wy = float(ky) / bh;
                const uint8_t* q[4] = {block(x0, y0), block(x0 + 1, y0), block(x0, y0 + 1), block(x0 + 1, y0 + 1)};
                float w[4] = {(1 - wx) * (1 - wy), wx * (1 - wy), (1 - wx) * wy, wx * wy};
                float A[4] = {0, 0, 0, 0}, B[4] = {0, 0, 0, 0};
                for (int i = 0; i < 4; i++) {
                    uint32_t c = le32(q[i] + 4);
                    Color a = colorA(c), b = colorB(c);
                    A[0] += a.r * w[i]; A[1] += a.g * w[i]; A[2] += a.b * w[i]; A[3] += a.a * w[i];
                    B[0] += b.r * w[i]; B[1] += b.g * w[i]; B[2] += b.b * w[i]; B[3] += b.a * w[i];
                }
                bool punch;
                int m = modulation(x, y, &punch);
                uint8_t* p = out + (size_t(y) * width + x) * 4;
                for (int c = 0; c < 4; c++) {
                    float v = (A[c] * (8 - m) + B[c] * m) / 8;
                    p[c] = uint8_t(v < 0 ? 0 : v > 255 ? 255 : v + 0.5f);
                }
                if (punch) p[3] = 0;
                if (!alpha) p[3] = 255;
            }
    }
};

bool isPVRTC(GLenum f) { return f >= kPVRTC_RGB_4 && f <= kPVRTC_RGBA_2; }

bool decodePVRTC(GLenum format, int width, int height, const void* data, int size, std::vector<uint8_t>& rgba) {
    Decoder d;
    d.two = format == kPVRTC_RGB_2 || format == kPVRTC_RGBA_2;
    d.bw = d.two ? 8 : 4;
    d.bh = 4;
    d.bx = (width + d.bw - 1) / d.bw;
    d.by = (height + d.bh - 1) / d.bh;
    if (d.bx < 2) d.bx = 2;
    if (d.by < 2) d.by = 2;
    if (size < d.bx * d.by * 8 || !data) return false;
    d.data = static_cast<const uint8_t*>(data);
    rgba.resize(size_t(width) * height * 4);
    d.decode(width, height, format == kPVRTC_RGBA_4 || format == kPVRTC_RGBA_2, rgba.data());
    return true;
}

uint64_t fnv(const void* data, int size, int width, int height, GLenum format) {
    uint64_t h = 14695981039346656037ull;
    auto mix = [&](unsigned char b) {
        h ^= b;
        h *= 1099511628211ull;
    };
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (int i = 0; i < size; i++) mix(p[i]);
    int parts[3] = {width, height, int(format)};
    for (int v : parts)
        for (int s = 0; s < 4; s++) mix(uint8_t(unsigned(v) >> (8 * s)));
    return h;
}

std::string cacheFile(uint64_t key, int width, int height, GLenum format) {
    char name[96];
    snprintf(name, sizeof name, "%016llx_%dx%d_%x.rgba", (unsigned long long)key, width, height, unsigned(format));
    return port::files::dataDir() + "/cache/pvrtc/" + name;
}

bool loadCached(const std::string& path, int width, int height, std::vector<uint8_t>* rgba) {
    std::vector<unsigned char> bytes;
    if (!port::files::read(path, &bytes)) return false;
    size_t need = size_t(width) * size_t(height) * 4;
    if (bytes.size() != need) return false;
    rgba->assign(bytes.begin(), bytes.end());
    return true;
}

void saveCached(const std::string& path, const std::vector<uint8_t>& rgba) {
    std::string root = port::files::dataDir();
    std::string dir = root + "/cache";
    if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) return;
    dir += "/pvrtc";
    if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) return;
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return;
    bool ok = fwrite(rgba.data(), 1, rgba.size(), f) == rgba.size();
    ok = fclose(f) == 0 && ok;
    if (!ok || rename(tmp.c_str(), path.c_str()) != 0) remove(tmp.c_str());
}

bool rgbaOf(GLenum format, int width, int height, const void* data, int size, std::vector<uint8_t>* rgba) {
    uint64_t key = fnv(data, size, width, height, format);
    std::string path = cacheFile(key, width, height, format);
    if (loadCached(path, width, height, rgba)) return true;
    if (!decodePVRTC(format, width, height, data, size, *rgba)) return false;
    saveCached(path, *rgba);
    return true;
}

}

namespace port {
namespace gfx {

uint64_t textureHash(const void* data, int size, int width, int height, unsigned format) {
    return fnv(data, size, width, height, format);
}

bool decodePVRTCTexture(unsigned format, int width, int height, const void* data, int size, std::vector<uint8_t>* rgba) {
    return isPVRTC(format) && rgbaOf(format, width, height, data, size, rgba);
}

}
}

namespace {

using TexImage = void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using TexSubImage = void (*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
using PixelStore = void (*)(GLenum, GLint);
using GetIntegerv = void (*)(GLenum, GLint*);

void withUnpack4(void (*upload)(const void*), const void* arg) {
    static auto get = reinterpret_cast<GetIntegerv>(port::gfx::glProc("glGetIntegerv"));
    static auto store = reinterpret_cast<PixelStore>(port::gfx::glProc("glPixelStorei"));
    GLint old = 4;
    get(GL_UNPACK_ALIGNMENT, &old);
    store(GL_UNPACK_ALIGNMENT, 4);
    upload(arg);
    store(GL_UNPACK_ALIGNMENT, old);
}

}

extern "C" {

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                            GLint border, GLsizei imageSize, const void* data) {
    MEDECOMP_GL_GUARD();
    if (isPVRTC(internalformat)) {
        static auto tex = reinterpret_cast<TexImage>(port::gfx::glProc("glTexImage2D"));
        std::vector<uint8_t> rgba;
        if (!rgbaOf(internalformat, width, height, data, imageSize, &rgba)) {
            fprintf(stderr, "medecomp: PVRTC %dx%d: данных %d байт — мало\n", width, height, imageSize);
            return;
        }
        struct Args { GLenum target; GLint level; GLsizei w, h; const uint8_t* px; } a = {target, level, width, height, rgba.data()};
        withUnpack4([](const void* p) {
            const Args* a = static_cast<const Args*>(p);
            tex(a->target, a->level, GL_RGBA, a->w, a->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, a->px);
        }, &a);
        return;
    }
    static auto real = reinterpret_cast<void (*)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*)>(
        port::gfx::glProc("glCompressedTexImage2D"));
    real(target, level, internalformat, width, height, border, imageSize, data);
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                               GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
    MEDECOMP_GL_GUARD();
    if (isPVRTC(format)) {
        static auto sub = reinterpret_cast<TexSubImage>(port::gfx::glProc("glTexSubImage2D"));
        std::vector<uint8_t> rgba;
        if (!rgbaOf(format, width, height, data, imageSize, &rgba)) return;
        struct Args { GLenum target; GLint level, x, y; GLsizei w, h; const uint8_t* px; } a = {target, level, xoffset, yoffset, width, height, rgba.data()};
        withUnpack4([](const void* p) {
            const Args* a = static_cast<const Args*>(p);
            sub(a->target, a->level, a->x, a->y, a->w, a->h, GL_RGBA, GL_UNSIGNED_BYTE, a->px);
        }, &a);
        return;
    }
    static auto real = reinterpret_cast<void (*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*)>(
        port::gfx::glProc("glCompressedTexSubImage2D"));
    real(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

}
