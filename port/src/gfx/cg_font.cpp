#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <cairo-ft.h>

#include <fstream>
#include <iterator>
#include <mutex>

#include "cg.h"

namespace port {
namespace gfx {

std::recursive_mutex& textMutex();

struct FontData {
    std::shared_ptr<std::vector<uint8_t>> bytes;
    FT_Face metrics = nullptr;
    cairo_font_face_t* face = nullptr;
    ~FontData();
};

namespace {

FT_Library library() {
    static FT_Library lib = [] {
        FT_Library l = nullptr;
        if (FT_Init_FreeType(&l) != 0) fprintf(stderr, "medecomp: FreeType не запускается\n");
        return l;
    }();
    return lib;
}

const cairo_user_data_key_t kFaceKey = {};

struct CairoHolder {
    FT_Face face;
    std::shared_ptr<std::vector<uint8_t>> bytes;
};

}

std::recursive_mutex& textMutex() {
    static std::recursive_mutex m;
    return m;
}

FontData::~FontData() {
    std::lock_guard<std::recursive_mutex> l(textMutex());
    if (face) cairo_font_face_destroy(face);
    if (metrics) FT_Done_Face(metrics);
}

std::shared_ptr<FontData> fontFromBytes(std::vector<uint8_t> data) {
    std::lock_guard<std::recursive_mutex> l(textMutex());
    auto f = std::make_shared<FontData>();
    f->bytes = std::make_shared<std::vector<uint8_t>>(std::move(data));
    if (f->bytes->empty() ||
        FT_New_Memory_Face(library(), f->bytes->data(), FT_Long(f->bytes->size()), 0, &f->metrics) != 0) {
        f->metrics = nullptr;
        fprintf(stderr, "medecomp: шрифт не читается (%zu байт)\n", f->bytes->size());
        return nullptr;
    }
    return f;
}

std::shared_ptr<FontData> fontFromFile(const std::string& path, int index) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return nullptr;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::lock_guard<std::recursive_mutex> l(textMutex());
    auto f = std::make_shared<FontData>();
    f->bytes = std::make_shared<std::vector<uint8_t>>(std::move(data));
    if (FT_New_Memory_Face(library(), f->bytes->data(), FT_Long(f->bytes->size()), index, &f->metrics) != 0) {
        f->metrics = nullptr;
        return nullptr;
    }
    return f;
}

int unitsPerEm(FontData& f) { return f.metrics->units_per_EM ? f.metrics->units_per_EM : 1000; }
int ascent(FontData& f) { return f.metrics->ascender; }
int descent(FontData& f) { return f.metrics->descender; }
int leading(FontData& f) {
    int gap = f.metrics->height - (f.metrics->ascender - f.metrics->descender);
    return gap > 0 ? gap : 0;
}

int advance(FontData& f, unsigned glyph) {
    std::lock_guard<std::recursive_mutex> l(textMutex());
    FT_Fixed a = 0;
    if (FT_Get_Advance(f.metrics, glyph, FT_LOAD_NO_SCALE, &a) != 0) return 0;
    return int(a);
}

unsigned glyphForChar(FontData& f, unsigned codepoint) {
    std::lock_guard<std::recursive_mutex> l(textMutex());
    return FT_Get_Char_Index(f.metrics, codepoint);
}

CGRect bbox(FontData& f) {
    const FT_BBox& b = f.metrics->bbox;
    CGRect r;
    r.origin.x = float(b.xMin);
    r.origin.y = float(b.yMin);
    r.size.width = float(b.xMax - b.xMin);
    r.size.height = float(b.yMax - b.yMin);
    return r;
}

cairo_font_face_t* cairoFace(FontData& f) {
    std::lock_guard<std::recursive_mutex> l(textMutex());
    if (f.face) return f.face;
    FT_Face face = nullptr;
    if (FT_New_Memory_Face(library(), f.bytes->data(), FT_Long(f.bytes->size()), f.metrics->face_index, &face) != 0)
        return nullptr;
    f.face = cairo_ft_font_face_create_for_ft_face(face, 0);
    cairo_font_face_set_user_data(f.face, &kFaceKey, new CairoHolder{face, f.bytes}, [](void* p) {
        CairoHolder* h = static_cast<CairoHolder*>(p);
        std::lock_guard<std::recursive_mutex> l(textMutex());
        FT_Done_Face(h->face);
        delete h;
    });
    return f.face;
}

}
}

struct CGFont : port::cf::Object {
    std::shared_ptr<port::gfx::FontData> data;
    CGFont();
};
namespace {
const port::cf::Class kFont = {"CGFont", [](port::cf::Object* o) { delete static_cast<CGFont*>(o); }, nullptr};
}
CGFont::CGFont() : port::cf::Object(&kFont) {}

namespace port {
namespace gfx {
FontData* fontOf(CGFont* font) { return font ? font->data.get() : nullptr; }
}
}

extern "C" {

CGFont* CGFontCreateWithDataProvider(void* provider) {
    auto data = port::gfx::fontFromBytes(port::gfx::providerBytes(provider));
    if (!data) return nullptr;
    CGFont* font = new CGFont();
    font->data = data;
    return font;
}

void CGFontRetain(CGFont* font) { CFRetain(font); }
void CGFontRelease(CGFont* font) { CFRelease(font); }

int CGFontGetUnitsPerEm(CGFont* font) { return font ? port::gfx::unitsPerEm(*font->data) : 1000; }
int CGFontGetAscent(CGFont* font) { return font ? port::gfx::ascent(*font->data) : 0; }
int CGFontGetDescent(CGFont* font) { return font ? port::gfx::descent(*font->data) : 0; }
int CGFontGetLeading(CGFont* font) { return font ? port::gfx::leading(*font->data) : 0; }

void CGFontGetFontBBox(CGRect* bbox, CGFont* font) {
    if (!bbox) return;
    *bbox = font ? port::gfx::bbox(*font->data) : CGRect{};
}

void CGFontGetGlyphAdvances(CGFont* font, const unsigned short* glyphs, int count, int* advances) {
    for (int i = 0; i < count; i++) advances[i] = font ? port::gfx::advance(*font->data, glyphs[i]) : 0;
}

}
