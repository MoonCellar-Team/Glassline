#include "png.h"

#include <stdio.h>
#include <string.h>
#include <zlib.h>

namespace port {
namespace gfx {

namespace {

uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }

bool inflateAll(const std::vector<uint8_t>& in, bool raw, std::vector<uint8_t>& out, size_t expected) {
    z_stream z;
    memset(&z, 0, sizeof z);
    if (inflateInit2(&z, raw ? -15 : 15) != Z_OK) return false;
    out.resize(expected);
    z.next_in = const_cast<Bytef*>(in.data());
    z.avail_in = static_cast<uInt>(in.size());
    z.next_out = out.data();
    z.avail_out = static_cast<uInt>(out.size());
    int r = inflate(&z, Z_FINISH);
    size_t got = out.size() - z.avail_out;
    inflateEnd(&z);
    return (r == Z_STREAM_END || r == Z_OK || r == Z_BUF_ERROR) && got == expected;
}

uint8_t paeth(int a, int b, int c) {
    int p = a + b - c, pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
    return uint8_t(pa <= pb && pa <= pc ? a : pb <= pc ? b : c);
}

bool unfilter(uint8_t* data, int rows, size_t rowBytes, int bpp) {
    std::vector<uint8_t> prev(rowBytes, 0);
    uint8_t* p = data;
    uint8_t* outRow = data;
    for (int y = 0; y < rows; y++) {
        int f = *p++;
        for (size_t i = 0; i < rowBytes; i++) {
            int a = i >= size_t(bpp) ? outRow[i - bpp] : 0, b = prev[i], c = i >= size_t(bpp) ? prev[i - bpp] : 0;
            int x = p[i];
            switch (f) {
            case 0: break;
            case 1: x += a; break;
            case 2: x += b; break;
            case 3: x += (a + b) >> 1; break;
            case 4: x += paeth(a, b, c); break;
            default: return false;
            }
            outRow[i] = uint8_t(x);
        }
        memcpy(prev.data(), outRow, rowBytes);
        p += rowBytes;
        outRow += rowBytes;
    }
    return true;
}

struct Header {
    int width, height, depth, color, interlace;
    bool iphone;
    std::vector<uint8_t> palette;
    int trnsGray = -1, trnsR = -1, trnsG = -1, trnsB = -1;
    bool straight = false;
};

int channels(int color) { return color == 0 ? 1 : color == 2 ? 3 : color == 3 ? 1 : color == 4 ? 2 : 4; }

int sample(const uint8_t* row, int x, int ch, int nch, int depth) {
    if (depth == 8) return row[x * nch + ch];
    if (depth == 16) return row[(x * nch + ch) * 2];
    int bit = x * depth;
    int v = (row[bit >> 3] >> (8 - depth - (bit & 7))) & ((1 << depth) - 1);
    return v;
}

int raw16(const uint8_t* row, int x, int ch, int nch) {
    const uint8_t* s = row + (x * nch + ch) * 2;
    return s[0] << 8 | s[1];
}

void putPixel(const Header& h, const uint8_t* row, int x, uint8_t* dst) {
    int nch = channels(h.color);
    int r, g, b, a = 255;
    switch (h.color) {
    case 0: {
        int v = sample(row, x, 0, 1, h.depth);
        int full = h.depth == 16 ? raw16(row, x, 0, 1) : v;
        if (full == h.trnsGray) a = 0;
        if (h.depth < 8) v = v * 255 / ((1 << h.depth) - 1);
        r = g = b = v;
        break;
    }
    case 2:
        r = sample(row, x, 0, 3, h.depth);
        g = sample(row, x, 1, 3, h.depth);
        b = sample(row, x, 2, 3, h.depth);
        if (h.trnsR >= 0) {
            bool key = h.depth == 16 ? raw16(row, x, 0, 3) == h.trnsR && raw16(row, x, 1, 3) == h.trnsG &&
                                           raw16(row, x, 2, 3) == h.trnsB
                                     : r == h.trnsR && g == h.trnsG && b == h.trnsB;
            if (key) a = 0;
        }
        break;
    case 3: {
        int i = sample(row, x, 0, 1, h.depth);
        if (size_t(i) * 4 + 3 < h.palette.size()) {
            r = h.palette[i * 4];
            g = h.palette[i * 4 + 1];
            b = h.palette[i * 4 + 2];
            a = h.palette[i * 4 + 3];
        } else {
            r = g = b = 0;
        }
        break;
    }
    case 4:
        r = g = b = sample(row, x, 0, 2, h.depth);
        a = sample(row, x, 1, 2, h.depth);
        break;
    default:
        r = sample(row, x, 0, 4, h.depth);
        g = sample(row, x, 1, 4, h.depth);
        b = sample(row, x, 2, 4, h.depth);
        a = sample(row, x, 3, nch, h.depth);
        break;
    }
    if (h.iphone) {
        dst[0] = uint8_t(b);
        dst[1] = uint8_t(g);
        dst[2] = uint8_t(r);
        dst[3] = uint8_t(a);
    } else if (h.straight) {
        dst[0] = uint8_t(r);
        dst[1] = uint8_t(g);
        dst[2] = uint8_t(b);
        dst[3] = uint8_t(a);
    } else {
        dst[0] = uint8_t((r * a + 127) / 255);
        dst[1] = uint8_t((g * a + 127) / 255);
        dst[2] = uint8_t((b * a + 127) / 255);
        dst[3] = uint8_t(a);
    }
}

}

bool decodePNG(const uint8_t* data, size_t size, Image& out, bool premultiplied) {
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (size < 8 || memcmp(data, sig, 8) != 0) return false;
    Header h = {};
    h.straight = !premultiplied;
    std::vector<uint8_t> idat;
    bool haveHeader = false;
    for (size_t p = 8; p + 12 <= size;) {
        uint32_t len = be32(data + p);
        const uint8_t* type = data + p + 4;
        const uint8_t* body = data + p + 8;
        if (p + 12 + len > size) return false;
        if (!memcmp(type, "CgBI", 4)) {
            h.iphone = true;
        } else if (!memcmp(type, "IHDR", 4) && len >= 13) {
            h.width = int(be32(body));
            h.height = int(be32(body + 4));
            h.depth = body[8];
            h.color = body[9];
            h.interlace = body[12];
            haveHeader = true;
        } else if (!memcmp(type, "PLTE", 4)) {
            h.palette.resize(len / 3 * 4);
            for (uint32_t i = 0; i < len / 3; i++) {
                memcpy(&h.palette[i * 4], body + i * 3, 3);
                h.palette[i * 4 + 3] = 255;
            }
        } else if (!memcmp(type, "tRNS", 4)) {
            if (h.color == 3)
                for (uint32_t i = 0; i < len && i * 4 + 3 < h.palette.size(); i++) h.palette[i * 4 + 3] = body[i];
            else if (h.color == 0 && len >= 2)
                h.trnsGray = body[0] << 8 | body[1];
            else if (h.color == 2 && len >= 6) {
                h.trnsR = body[0] << 8 | body[1];
                h.trnsG = body[2] << 8 | body[3];
                h.trnsB = body[4] << 8 | body[5];
                if (h.depth == 8) {
                    h.trnsR &= 0xff;
                    h.trnsG &= 0xff;
                    h.trnsB &= 0xff;
                }
            }
        } else if (!memcmp(type, "IDAT", 4)) {
            idat.insert(idat.end(), body, body + len);
        } else if (!memcmp(type, "IEND", 4)) {
            break;
        }
        p += 12 + len;
    }
    if (!haveHeader || h.width <= 0 || h.height <= 0 || h.width > 16384 || h.height > 16384) return false;
    if (h.color == 0 && h.depth == 8 && h.trnsGray > 255) h.trnsGray &= 0xff;
    int nch = channels(h.color);
    int bitsPerPixel = nch * h.depth;
    int bpp = bitsPerPixel < 8 ? 1 : bitsPerPixel / 8;

    struct Pass { int x0, y0, dx, dy; };
    static const Pass adam7[7] = {{0, 0, 8, 8}, {4, 0, 8, 8}, {0, 4, 4, 8}, {2, 0, 4, 4},
                                  {0, 2, 2, 4}, {1, 0, 2, 2}, {0, 1, 1, 2}};
    static const Pass whole = {0, 0, 1, 1};
    const Pass* passes = h.interlace ? adam7 : &whole;
    int passCount = h.interlace ? 7 : 1;
    size_t expected = 0;
    for (int i = 0; i < passCount; i++) {
        const Pass& ps = passes[i];
        int pw = (h.width - ps.x0 + ps.dx - 1) / ps.dx, ph = (h.height - ps.y0 + ps.dy - 1) / ps.dy;
        if (pw > 0 && ph > 0) expected += size_t(ph) * (1 + (size_t(pw) * bitsPerPixel + 7) / 8);
    }
    std::vector<uint8_t> raw;
    if (!inflateAll(idat, h.iphone, raw, expected)) return false;

    out.width = h.width;
    out.height = h.height;
    out.rgba.assign(size_t(h.width) * h.height * 4, 0);
    uint8_t* p = raw.data();
    for (int i = 0; i < passCount; i++) {
        const Pass& ps = passes[i];
        int pw = (h.width - ps.x0 + ps.dx - 1) / ps.dx, ph = (h.height - ps.y0 + ps.dy - 1) / ps.dy;
        if (pw <= 0 || ph <= 0) continue;
        size_t rowBytes = (size_t(pw) * bitsPerPixel + 7) / 8;
        if (!unfilter(p, ph, rowBytes, bpp)) return false;
        for (int y = 0; y < ph; y++) {
            const uint8_t* row = p + size_t(y) * rowBytes;
            uint8_t* dst = &out.rgba[(size_t(ps.y0 + y * ps.dy) * h.width) * 4];
            for (int x = 0; x < pw; x++) putPixel(h, row, x, dst + size_t(ps.x0 + x * ps.dx) * 4);
        }
        p += size_t(ph) * (rowBytes + 1);
    }
    return true;
}

namespace {

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

}

bool encodePNG(int width, int height, const uint8_t* rgba, std::string* png) {
    if (width <= 0 || height <= 0 || !rgba) return false;
    std::string raw;
    raw.reserve(size_t(height) * (size_t(width) * 4 + 1));
    for (int y = 0; y < height; y++) {
        raw += '\0';
        raw.append(reinterpret_cast<const char*>(rgba + size_t(y) * width * 4), size_t(width) * 4);
    }
    uLongf size = compressBound(uLong(raw.size()));
    std::string z(size, '\0');
    if (compress2(reinterpret_cast<Bytef*>(&z[0]), &size, reinterpret_cast<const Bytef*>(raw.data()), uLong(raw.size()),
                  6) != Z_OK)
        return false;
    z.resize(size);
    std::string ihdr;
    put32(ihdr, uint32_t(width));
    put32(ihdr, uint32_t(height));
    ihdr += std::string("\x08\x06\x00\x00\x00", 5);
    std::string out = "\x89PNG\r\n\x1a\n";
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", "");
    *png = std::move(out);
    return true;
}

bool writePNG(const std::string& path, int width, int height, const uint8_t* rgba) {
    std::string out;
    if (!encodePNG(width, height, rgba, &out)) return false;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    return fclose(f) == 0 && ok;
}

}
}
