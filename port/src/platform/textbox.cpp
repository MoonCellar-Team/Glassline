#include <stdio.h>
#include <string.h>

#include <mutex>
#include <string>
#include <vector>

#include <SDL.h>
#include <fontconfig/fontconfig.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"

#include "port/brand.h"
#include "port/cf.h"
#include "port/objc.h"
#include "port/platform.h"
#include "runtime/midp/String.h"
#include "runtime/midp/TextBox.h"
#include "runtime/midp/TextBoxImplementationIPhone.h"

using port::objc::Class;
using port::objc::Message;
using port::objc::NSObjectClass;
using port::objc::Object;

namespace {

const char* kTitle = MEDECOMP_PORT_NAME;

struct FieldObject : Object {
    midp::TextBoxImplementationIPhone* impl = nullptr;
    std::u16string text;
    std::string placeholder;
    const void* cached = nullptr;
    FieldObject();
    ~FieldObject() {
        if (cached) CFRelease(cached);
    }
};

const Class kUITextField = {{"UITextField", port::objc::destroyAs<FieldObject>, nullptr}, &NSObjectClass, {}};
FieldObject::FieldObject() : Object(&kUITextField) {}

struct DelegateObject : Object {
    midp::TextBoxImplementationIPhone* impl = nullptr;
    DelegateObject();
};

FieldObject* s_active;

void showTitle() {
    if (!s_active) {
        port::platform::setWindowTitle(kTitle);
        return;
    }
    std::string t = std::string(kTitle) + " — ";
    if (!s_active->placeholder.empty()) t += s_active->placeholder + ": ";
    t += port::cf::toUtf8(s_active->text) + "▏";
    port::platform::setWindowTitle(t);
}

void deactivate(FieldObject* field) {
    if (!field || s_active != field) return;
    s_active = nullptr;
    port::platform::stopTextInput();
    showTitle();
}

void* createTextField(Object* self, Message&) {
    DelegateObject* d = static_cast<DelegateObject*>(self);
    FieldObject* field = new FieldObject();
    field->impl = d->impl;
    if (midp::String* title = d->impl->getTextBox()->getTitle())
        field->placeholder = port::cf::utf8(title->getCFString());
    d->impl->m_field = field;
    return nullptr;
}

void* showNotify(Object* self, Message& m) {
    FieldObject* field = static_cast<FieldObject*>(va_arg(*m.args, void*));
    if (!field) return nullptr;
    s_active = field;
    port::platform::startTextInput();
    showTitle();
    return nullptr;
}

void* hideNotify(Object*, Message& m) {
    deactivate(static_cast<FieldObject*>(va_arg(*m.args, void*)));
    return nullptr;
}

void* remove(Object*, Message& m) {
    FieldObject* field = static_cast<FieldObject*>(va_arg(*m.args, void*));
    deactivate(field);
    if (field) CFRelease(field);
    return nullptr;
}

const Class kTextBoxDelegate = {
    {"TextBoxDelegate", port::objc::destroyAs<DelegateObject>, nullptr},
    &NSObjectClass,
    {
        {"createTextField", createTextField},
        {"showNotify:", showNotify},
        {"hideNotify:", hideNotify},
        {"remove:", remove},
    },
};

DelegateObject::DelegateObject() : Object(&kTextBoxDelegate) {}

std::mutex s_lineMu;
const void* s_lineOwner;
std::u16string s_lineText;
size_t s_lineLimit;
int s_lineKey;

void eraseLast(std::u16string& t) {
    if (t.empty()) return;
    bool low = t.back() >= 0xDC00 && t.back() < 0xE000;
    t.pop_back();
    if (low && !t.empty() && t.back() >= 0xD800 && t.back() < 0xDC00) t.pop_back();
}

}

extern "C" const char* medecomp_line(const void* owner, int limit, int* key) {
    static std::string utf8;
    bool opened = false;
    {
        std::lock_guard<std::mutex> lock(s_lineMu);
        if (s_lineOwner != owner) {
            s_lineOwner = owner;
            s_lineText.clear();
            s_lineKey = 0;
            opened = true;
        }
        s_lineLimit = size_t(limit);
        *key = s_lineKey;
        s_lineKey = 0;
        utf8 = port::cf::toUtf8(s_lineText);
    }
    if (opened) port::objc::onMainThread([] { port::platform::startTextInput(); }, false);
    return utf8.c_str();
}

extern "C" void medecomp_line_close(const void* owner) {
    {
        std::lock_guard<std::mutex> lock(s_lineMu);
        if (s_lineOwner != owner) return;
        s_lineOwner = nullptr;
    }
    port::objc::onMainThread([] { port::platform::stopTextInput(); }, false);
}

extern "C" {

void* TextBoxDelegate_alloc() __asm__("_ZN12_GLOBAL__N_115TextBoxDelegate5allocEv");
void* TextBoxDelegate_alloc() { return new DelegateObject(); }

void* TextBoxDelegate_initWithImplementation(void* self, midp::TextBoxImplementationIPhone* impl)
    __asm__("_ZN12_GLOBAL__N_115TextBoxDelegate22initWithImplementationEPN4midp27TextBoxImplementationIPhoneE");
void* TextBoxDelegate_initWithImplementation(void* self, midp::TextBoxImplementationIPhone* impl) {
    static_cast<DelegateObject*>(self)->impl = impl;
    return self;
}

void* UITextField_text(void* self) __asm__("_ZN12_GLOBAL__N_111UITextField4textEv");
void* UITextField_text(void* self) {
    FieldObject* field = static_cast<FieldObject*>(self);
    if (field->cached) CFRelease(field->cached);
    field->cached = port::cf::makeString(field->text);
    return const_cast<void*>(field->cached);
}

}

namespace port {
namespace platform {

bool textInputActive() { return s_active != nullptr; }

bool textLineActive() {
    std::lock_guard<std::mutex> lock(s_lineMu);
    return s_lineOwner != nullptr;
}

void textInput(const char* utf8) {
    {
        std::lock_guard<std::mutex> lock(s_lineMu);
        if (s_lineOwner) {
            s_lineText += port::cf::fromUtf8(utf8, strlen(utf8));
            while (s_lineText.size() > s_lineLimit) eraseLast(s_lineText);
            return;
        }
    }
    if (!s_active) return;
    s_active->text += port::cf::fromUtf8(utf8, strlen(utf8));
    showTitle();
}

void textKey(int key) {
    {
        std::lock_guard<std::mutex> lock(s_lineMu);
        if (s_lineOwner) {
            if (key == kKeyBackspace) eraseLast(s_lineText);
            else s_lineKey = key;
            return;
        }
    }
    if (!s_active) return;
    if (key == kKeyBackspace) {
        eraseLast(s_active->text);
        showTitle();
    } else if (key == kKeyReturn) {
        s_active->impl->endEditing();
    }
}

namespace {

stbtt_fontinfo* fieldFont() {
    static stbtt_fontinfo info;
    static std::vector<unsigned char> bytes;
    static int state = 0;
    if (state != 0) return state == 1 ? &info : nullptr;
    state = 2;
    if (!FcInit()) return nullptr;
    FcPattern* pattern = FcNameParse(reinterpret_cast<const FcChar8*>("sans-serif:lang=ru"));
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern* match = FcFontMatch(nullptr, pattern, &result);
    FcChar8* file = nullptr;
    if (match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
        FILE* f = fopen(reinterpret_cast<const char*>(file), "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long n = ftell(f);
            if (n > 0 && n < 40 * 1024 * 1024) {
                bytes.resize(size_t(n));
                fseek(f, 0, SEEK_SET);
                if (fread(bytes.data(), 1, bytes.size(), f) == bytes.size() &&
                    stbtt_InitFont(&info, bytes.data(), stbtt_GetFontOffsetForIndex(bytes.data(), 0)))
                    state = 1;
            }
            fclose(f);
        }
    }
    if (match) FcPatternDestroy(match);
    FcPatternDestroy(pattern);
    return state == 1 ? &info : nullptr;
}

void blitGlyph(std::vector<unsigned char>& rgba, int stride, int penX, int baseline, unsigned char* bmp, int bw,
               int bh, int xoff, int yoff) {
    for (int y = 0; y < bh; y++) {
        int dy = baseline + yoff + y;
        if (dy < 0 || dy >= int(rgba.size() / 4 / stride)) continue;
        for (int x = 0; x < bw; x++) {
            int dx = penX + xoff + x;
            if (dx < 0 || dx >= stride) continue;
            unsigned char a = bmp[y * bw + x];
            unsigned char* p = &rgba[(size_t(dy) * stride + dx) * 4];
            p[0] = (unsigned char)((p[0] * (255 - a) + 255 * a) / 255);
            p[1] = (unsigned char)((p[1] * (255 - a) + 255 * a) / 255);
            p[2] = (unsigned char)((p[2] * (255 - a) + 255 * a) / 255);
            p[3] = 255;
        }
    }
}

}

bool textFieldImage(std::vector<unsigned char>* rgba, int* width, int* height) {
    if (!s_active || !rgba || !width || !height) return false;
    stbtt_fontinfo* font = fieldFont();
    std::string utf8 = port::cf::toUtf8(s_active->text);
    const float pixel = 32.0f;
    float scale = font ? stbtt_ScaleForPixelHeight(font, pixel) : 1;
    int ascent = 0, descent = 0, gap = 0;
    if (font) stbtt_GetFontVMetrics(font, &ascent, &descent, &gap);
    int baseline = 28 + (font ? int(ascent * scale) : 32);
    int pen = 28;
    struct Glyph {
        int x, yoff, w, h, advance;
        std::vector<unsigned char> bmp;
    };
    std::vector<Glyph> glyphs;
    const unsigned char* s = reinterpret_cast<const unsigned char*>(utf8.c_str());
    for (size_t i = 0; i < utf8.size();) {
        unsigned cp = s[i];
        int n = 1;
        if (cp >= 0xF0 && i + 4 <= utf8.size()) {
            cp = ((s[i] & 7) << 18) | ((s[i + 1] & 63) << 12) | ((s[i + 2] & 63) << 6) | (s[i + 3] & 63);
            n = 4;
        } else if (cp >= 0xE0 && i + 3 <= utf8.size()) {
            cp = ((s[i] & 15) << 12) | ((s[i + 1] & 63) << 6) | (s[i + 2] & 63);
            n = 3;
        } else if (cp >= 0xC0 && i + 2 <= utf8.size()) {
            cp = ((s[i] & 31) << 6) | (s[i + 1] & 63);
            n = 2;
        }
        i += n;
        Glyph g;
        g.x = pen;
        g.w = g.h = 0;
        g.yoff = 0;
        g.advance = 18;
        if (font) {
            int ax = 0, lsb = 0, xoff = 0, yoff = 0, bw = 0, bh = 0;
            stbtt_GetCodepointHMetrics(font, int(cp), &ax, &lsb);
            unsigned char* bmp = stbtt_GetCodepointBitmap(font, scale, scale, int(cp), &bw, &bh, &xoff, &yoff);
            g.w = bw;
            g.h = bh;
            g.yoff = yoff;
            g.advance = int(ax * scale + 0.5f);
            if (g.advance < 8) g.advance = 8;
            g.x = pen + xoff;
            if (bmp && bw > 0 && bh > 0) {
                g.bmp.assign(bmp, bmp + bw * bh);
                stbtt_FreeBitmap(bmp, nullptr);
            } else {
                g.w = g.h = 0;
                if (bmp) stbtt_FreeBitmap(bmp, nullptr);
            }
        }
        glyphs.push_back(std::move(g));
        pen += glyphs.back().advance;
    }
    bool caret = (SDL_GetTicks() / 500) % 2 == 0;
    int imgW = pen + 36;
    if (imgW < 520) imgW = 520;
    int imgH = 96;
    rgba->assign(size_t(imgW) * imgH * 4, 0);
    for (int i = 0; i < imgW * imgH; i++) {
        (*rgba)[i * 4 + 0] = 24;
        (*rgba)[i * 4 + 1] = 24;
        (*rgba)[i * 4 + 2] = 28;
        (*rgba)[i * 4 + 3] = 255;
    }
    for (int x = 0; x < imgW; x++) {
        for (int y = 0; y < 3; y++) {
            (*rgba)[(y * imgW + x) * 4 + 0] = (*rgba)[(y * imgW + x) * 4 + 1] = (*rgba)[(y * imgW + x) * 4 + 2] = 220;
            (*rgba)[((imgH - 1 - y) * imgW + x) * 4 + 0] = (*rgba)[((imgH - 1 - y) * imgW + x) * 4 + 1] =
                (*rgba)[((imgH - 1 - y) * imgW + x) * 4 + 2] = 220;
        }
    }
    for (const Glyph& g : glyphs) blitGlyph(*rgba, imgW, g.x, baseline, const_cast<unsigned char*>(g.bmp.data()), g.w, g.h, 0, g.yoff);
    if (caret) {
        for (int y = baseline - 28; y < baseline + 6; y++)
            for (int x = pen; x < pen + 3 && x < imgW; x++)
                if (y >= 0 && y < imgH) {
                    unsigned char* p = &(*rgba)[(size_t(y) * imgW + x) * 4];
                    p[0] = p[1] = p[2] = 255;
                }
    }
    *width = imgW;
    *height = imgH;
    return true;
}

}
}
