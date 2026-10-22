#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontconfig/fontconfig.h>

#include <map>
#include <mutex>
#include <string>

#include "cg.h"
#include "port/objc.h"
#include "sdk/UIKit.h"

namespace port {
namespace gfx {
std::recursive_mutex& textMutex();
}
}

using port::gfx::FontData;
using port::objc::Message;
using port::objc::Object;

namespace {

struct FontObject : Object {
    std::shared_ptr<FontData> data;
    float size = 14;
    FontObject();
};

void* floatResult(float f) {
    union {
        void* p;
        float f;
    } u;
    u.p = nullptr;
    u.f = f;
    return u.p;
}

float px(FontObject* f, int units) { return float(units) * f->size / float(port::gfx::unitsPerEm(*f->data)); }

FontObject* font(const std::string& name, float size);

void* fontAscender(Object* self, Message&) {
    FontObject* f = static_cast<FontObject*>(self);
    return floatResult(px(f, port::gfx::ascent(*f->data)));
}
void* fontDescender(Object* self, Message&) {
    FontObject* f = static_cast<FontObject*>(self);
    return floatResult(px(f, port::gfx::descent(*f->data)));
}
float lineHeight(FontObject* f) {
    return px(f, port::gfx::ascent(*f->data) - port::gfx::descent(*f->data) + port::gfx::leading(*f->data));
}
void* fontLeading(Object* self, Message&) { return floatResult(lineHeight(static_cast<FontObject*>(self))); }
void* fontPointSize(Object* self, Message&) { return floatResult(static_cast<FontObject*>(self)->size); }
void* fontWithSize(Object* self, Message& m);

const port::objc::Class kUIFont = {
    {"UIFont", port::objc::destroyAs<FontObject>, nullptr},
    &port::objc::NSObjectClass,
    {
        {"ascender", fontAscender},
        {"descender", fontDescender},
        {"leading", fontLeading},
        {"lineHeight", fontLeading},
        {"pointSize", fontPointSize},
        {"fontWithSize:", fontWithSize},
    },
};
FontObject::FontObject() : Object(&kUIFont) {}

std::map<std::pair<std::string, float>, FontObject*> s_fonts;
std::map<std::string, std::shared_ptr<FontData>> s_faces;
std::map<const FontObject*, std::string> s_names;
std::map<std::pair<std::string, int>, std::shared_ptr<FontData>> s_files;

std::shared_ptr<FontData> fontFile(const char* file, int index) {
    auto key = std::make_pair(std::string(file), index);
    auto it = s_files.find(key);
    if (it != s_files.end()) return it->second;
    return s_files[key] = port::gfx::fontFromFile(file, index);
}

std::shared_ptr<FontData> face(const std::string& name) {
    auto it = s_faces.find(name);
    if (it != s_faces.end()) return it->second;
    std::string family = name.substr(0, name.find('-'));
    std::string style = name.find('-') == std::string::npos ? "" : name.substr(name.find('-') + 1);
    std::string spaced;
    for (size_t i = 0; i < family.size(); i++) {
        if (i && isupper((unsigned char)family[i]) && islower((unsigned char)family[i - 1])) spaced += ' ';
        spaced += family[i];
    }
    bool bold = style.find("Bold") != std::string::npos;
    bool italic = style.find("Oblique") != std::string::npos || style.find("Italic") != std::string::npos;
    if (spaced == "Helvetica" || spaced == "Helvetica Neue") spaced = "Nimbus Sans";
    std::shared_ptr<FontData> data;
    if (FcInit()) {
        FcPattern* p = FcPatternCreate();
        FcPatternAddString(p, FC_FAMILY, reinterpret_cast<const FcChar8*>(spaced.c_str()));
        FcPatternAddInteger(p, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
        FcPatternAddInteger(p, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
        FcConfigSubstitute(nullptr, p, FcMatchPattern);
        FcDefaultSubstitute(p);
        FcResult r;
        if (FcPattern* match = FcFontMatch(nullptr, p, &r)) {
            FcChar8* file = nullptr;
            int index = 0;
            if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
                FcPatternGetInteger(match, FC_INDEX, 0, &index);
                data = fontFile(reinterpret_cast<const char*>(file), index);
                fprintf(stderr, "medecomp: шрифт %s -> %s\n", name.c_str(), file);
            }
            FcPatternDestroy(match);
        }
        FcPatternDestroy(p);
    }
    if (!data) fprintf(stderr, "medecomp: нет шрифта для %s\n", name.c_str());
    s_faces[name] = data;
    return data;
}

FontObject* font(const std::string& name, float size) {
    std::lock_guard<std::recursive_mutex> l(port::gfx::textMutex());
    auto key = std::make_pair(name, size);
    auto it = s_fonts.find(key);
    if (it != s_fonts.end()) return it->second;
    std::shared_ptr<FontData> data = face(name);
    FontObject* f = nullptr;
    if (data) {
        f = new FontObject();
        f->data = data;
        f->size = size;
        s_names[f] = name;
    }
    s_fonts[key] = f;
    return f;
}

void* fontWithSize(Object* self, Message& m) {
    float size = float(va_arg(*m.args, double));
    return font(s_names[static_cast<FontObject*>(self)], size);
}

extern "C" const char* medecomp_language(void);

const char* uiLanguage() {
    const char* set = medecomp_language();
    if (set && *set) return set;
    const char* env = getenv("MEDECOMP_LANG");
    return env ? env : "";
}

const char* scriptLang(unsigned cp) {
    if ((cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0x1100 && cp <= 0x11FF) || (cp >= 0x3130 && cp <= 0x318F))
        return "ko";
    if ((cp >= 0x3040 && cp <= 0x30FF) || (cp >= 0x31F0 && cp <= 0x31FF)) return "ja";
    bool cjk = (cp >= 0x3400 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0x3000 && cp <= 0x303F) ||
               (cp >= 0xFF00 && cp <= 0xFFEF);
    if (!cjk) return nullptr;
    std::string lang = uiLanguage();
    if (lang.compare(0, 2, "ko") == 0) return "ko";
    if (lang.find("TW") != std::string::npos || lang.find("HK") != std::string::npos ||
        lang.find("Hant") != std::string::npos)
        return "zh-tw";
    if (lang.compare(0, 2, "zh") == 0) return "zh-cn";
    return "ja";
}

std::map<std::string, std::shared_ptr<FontData>> s_scriptFonts;
std::map<std::string, std::shared_ptr<FontData>> s_charFonts;

std::shared_ptr<FontData> faceFor(unsigned cp, bool bold, bool italic) {
    const char* lang = scriptLang(cp);
    std::string key = std::string(lang ? lang : "*") + (bold ? "/b" : "/r") + (italic ? "/i" : "/n");
    auto known = s_scriptFonts.find(key);
    if (known != s_scriptFonts.end() && port::gfx::glyphForChar(*known->second, cp)) return known->second;
    std::string charKey = key + "/" + std::to_string(cp);
    auto seen = s_charFonts.find(charKey);
    if (seen != s_charFonts.end()) return seen->second;
    if (!FcInit()) return nullptr;
    FcPattern* p = FcPatternCreate();
    FcCharSet* cs = FcCharSetCreate();
    FcCharSetAddChar(cs, cp);
    FcPatternAddCharSet(p, FC_CHARSET, cs);
    FcPatternAddString(p, FC_FAMILY, reinterpret_cast<const FcChar8*>("sans-serif"));
    if (lang) FcPatternAddString(p, FC_LANG, reinterpret_cast<const FcChar8*>(lang));
    FcPatternAddInteger(p, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(p, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcPatternAddBool(p, FC_SCALABLE, FcTrue);
    FcConfigSubstitute(nullptr, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    FcResult r;
    std::shared_ptr<FontData> data;
    if (FcPattern* match = FcFontMatch(nullptr, p, &r)) {
        FcChar8* file = nullptr;
        int index = 0;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
            FcPatternGetInteger(match, FC_INDEX, 0, &index);
            data = fontFile(reinterpret_cast<const char*>(file), index);
            if (data && port::gfx::glyphForChar(*data, cp)) {
                static std::map<std::string, bool> logged;
                if (!logged[reinterpret_cast<const char*>(file)]) {
                    logged[reinterpret_cast<const char*>(file)] = true;
                    fprintf(stderr, "medecomp: шрифт CJK -> %s\n", file);
                }
            } else {
                data.reset();
            }
        }
        FcPatternDestroy(match);
    }
    FcCharSetDestroy(cs);
    FcPatternDestroy(p);
    s_charFonts[charKey] = data;
    if (data && known == s_scriptFonts.end()) s_scriptFonts[key] = data;
    return data;
}

struct GlyphRun {
    std::shared_ptr<FontData> data;
    std::vector<unsigned short> glyphs;
    float width = 0;
};

float advancePx(FontData& data, unsigned glyph, float size) {
    return float(port::gfx::advance(data, glyph)) * size / float(port::gfx::unitsPerEm(data));
}

std::vector<GlyphRun> glyphsOf(const void* string, FontObject* f, float* width) {
    std::u16string t = port::cf::text(string);
    std::string name = s_names[f];
    std::string style = name.find('-') == std::string::npos ? "" : name.substr(name.find('-') + 1);
    bool bold = style.find("Bold") != std::string::npos;
    bool italic = style.find("Oblique") != std::string::npos || style.find("Italic") != std::string::npos;
    std::vector<GlyphRun> runs;
    float total = 0;
    for (size_t i = 0; i < t.size(); i++) {
        unsigned cp = t[i];
        if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < t.size() && t[i + 1] >= 0xDC00 && t[i + 1] < 0xE000)
            cp = 0x10000 + ((cp - 0xD800) << 10) + (t[++i] - 0xDC00);
        std::shared_ptr<FontData> data = f->data;
        unsigned g = port::gfx::glyphForChar(*data, cp);
        if (g == 0 && cp >= 0x80) {
            if (std::shared_ptr<FontData> fallback = faceFor(cp, bold, italic)) {
                data = fallback;
                g = port::gfx::glyphForChar(*data, cp);
            }
        }
        if (runs.empty() || runs.back().data != data) runs.push_back({data, {}, 0});
        runs.back().glyphs.push_back((unsigned short)g);
        float adv = advancePx(*data, g, f->size);
        runs.back().width += adv;
        total += adv;
    }
    *width = total;
    return runs;
}

thread_local std::vector<CGContext*> t_contexts;

bool stringMessage(Message& m, void** result) {
    bool size = !strcmp(m.sel, "sizeWithFont:");
    bool draw = !strcmp(m.sel, "drawAtPoint:withFont:");
    if ((!size && !draw) || !port::cf::isString(m.self)) return false;
    double x = 0, y = 0;
    if (draw) {
        x = va_arg(*m.args, double);
        y = va_arg(*m.args, double);
    }
    FontObject* f = static_cast<FontObject*>(va_arg(*m.args, void*));
    CGSize out = {0, 0};
    if (f && f->data) {
        std::lock_guard<std::recursive_mutex> l(port::gfx::textMutex());
        float width = 0;
        std::vector<GlyphRun> runs = glyphsOf(m.self, f, &width);
        out.width = ceilf(width);
        out.height = ceilf(lineHeight(f));
        if (draw && !t_contexts.empty()) {
            const float matrix[4] = {1, 0, 0, -1};
            float baseline = float(y) + px(f, port::gfx::ascent(*f->data));
            float pen = float(x);
            for (const GlyphRun& run : runs) {
                if (run.glyphs.empty()) continue;
                port::gfx::showGlyphs(t_contexts.back(), *run.data, f->size, matrix, pen, baseline, run.glyphs.data(),
                                      int(run.glyphs.size()));
                pen += run.width;
            }
        }
    }
    if (m.stret) *static_cast<CGSize*>(m.stret) = out;
    *result = nullptr;
    return true;
}

const bool s_registered = (port::objc::addResponder(stringMessage), true);

}

float UIFont::systemFontSize() { return 14.0f; }

UIFont* UIFont::systemFontOfSize(float size) { return reinterpret_cast<UIFont*>(font("Helvetica", size)); }

UIFont* UIFont::fontWithName_size(NSString* name, float size) {
    return reinterpret_cast<UIFont*>(font(port::cf::utf8(name), size));
}

extern "C" {

void UIGraphicsPushContext(CGContext* context) { t_contexts.push_back(context); }

void UIGraphicsPopContext() {
    if (!t_contexts.empty()) t_contexts.pop_back();
}

}
