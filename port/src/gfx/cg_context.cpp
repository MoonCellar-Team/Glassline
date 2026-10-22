#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>

#include "cg.h"

namespace port {
namespace gfx {
std::recursive_mutex& textMutex();
struct FontData;
FontData* fontOf(CGFont* font);
}
}

namespace {

struct GState {
    float fill[4] = {0, 0, 0, 1};
    float stroke[4] = {0, 0, 0, 1};
    CGFont* font = nullptr;
    float fontSize = 12;
    float text[4] = {1, 0, 0, 1};
    int textMode = 0;
    bool antialias = true;
    int interpolation = 0;
    int blend = 0;
    bool shadow = false;
    float shadowDx = 0, shadowDy = 0, shadowBlur = 0;
    float shadowColor[4] = {0, 0, 0, 0};
};

}

struct CGContext : port::cf::Object {
    int width = 0, height = 0;
    uint8_t* data = nullptr;
    bool ownsData = false;
    cairo_surface_t* surface = nullptr;
    cairo_t* cr = nullptr;
    GState gs;
    std::vector<GState> saved;
    float baseScale = 1;
    std::vector<CGFont*> fonts;
    CGContext();
    ~CGContext() {
        if (cr) cairo_destroy(cr);
        if (surface) cairo_surface_destroy(surface);
        for (CGFont* f : fonts) CFRelease(f);
        if (ownsData) delete[] data;
    }
};

namespace {
const port::cf::Class kContext = {"CGContext", [](port::cf::Object* o) { delete static_cast<CGContext*>(o); },
                                  nullptr};

void setSource(cairo_t* cr, const float c[4]) { cairo_set_source_rgba(cr, c[2], c[1], c[0], c[3]); }

cairo_operator_t blendOp(int mode) {
    switch (mode) {
    case 16: return CAIRO_OPERATOR_CLEAR;
    case 17: return CAIRO_OPERATOR_SOURCE;
    default: return CAIRO_OPERATOR_OVER;
    }
}

void prepare(CGContext* c, cairo_t* t) {
    cairo_set_antialias(t, c->gs.antialias ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(t, 1.0);
}

cairo_surface_t* shadowMask(cairo_surface_t* layer, float blur) {
    int w = cairo_image_surface_get_width(layer), h = cairo_image_surface_get_height(layer);
    int ls = cairo_image_surface_get_stride(layer);
    const uint8_t* src = cairo_image_surface_get_data(layer);
    cairo_surface_t* mask = cairo_image_surface_create(CAIRO_FORMAT_A8, w, h);
    int ms = cairo_image_surface_get_stride(mask);
    uint8_t* dst = cairo_image_surface_get_data(mask);
    std::vector<float> a(size_t(w) * h), tmp(size_t(w) * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) a[size_t(y) * w + x] = float(*reinterpret_cast<const uint32_t*>(src + y * ls + x * 4) >> 24);
    float sigma = blur / 2;
    int r = sigma > 0 ? int(floorf((sqrtf(4 * sigma * sigma + 1) - 1) / 2 + 0.5f)) : 0;
    for (int pass = 0; pass < 3 && r > 0; pass++) {
        float norm = 1.0f / (2 * r + 1);
        for (int y = 0; y < h; y++) {
            const float* row = &a[size_t(y) * w];
            float sum = 0;
            for (int x = -r; x <= r; x++) sum += row[x < 0 ? 0 : x >= w ? w - 1 : x] * (x >= 0 && x < w);
            for (int x = 0; x < w; x++) {
                tmp[size_t(y) * w + x] = sum * norm;
                int add = x + r + 1, sub = x - r;
                if (add < w) sum += row[add];
                if (sub >= 0) sum -= row[sub];
            }
        }
        for (int x = 0; x < w; x++) {
            float sum = 0;
            for (int y = 0; y <= r && y < h; y++) sum += tmp[size_t(y) * w + x];
            for (int y = 0; y < h; y++) {
                a[size_t(y) * w + x] = sum * norm;
                int add = y + r + 1, sub = y - r;
                if (add < h) sum += tmp[size_t(add) * w + x];
                if (sub >= 0) sum -= tmp[size_t(sub) * w + x];
            }
        }
    }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float v = a[size_t(y) * w + x];
            dst[y * ms + x] = uint8_t(v < 0 ? 0 : v > 255 ? 255 : v + 0.5f);
        }
    cairo_surface_mark_dirty(mask);
    return mask;
}

template <class F>
void draw(CGContext* c, F&& fn) {
    cairo_t* cr = c->cr;
    cairo_surface_mark_dirty(c->surface);
    cairo_set_operator(cr, blendOp(c->gs.blend));
    prepare(c, cr);
    if (!c->gs.shadow || c->gs.shadowColor[3] <= 0) {
        fn(cr);
    } else {
        cairo_surface_t* record = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
        cairo_t* rc = cairo_create(record);
        cairo_matrix_t m;
        cairo_get_matrix(cr, &m);
        cairo_set_matrix(rc, &m);
        prepare(c, rc);
        fn(rc);
        cairo_destroy(rc);
        double ix = 0, iy = 0, iw = 0, ih = 0;
        cairo_recording_surface_ink_extents(record, &ix, &iy, &iw, &ih);
        float blur = c->gs.shadowBlur * c->baseScale;
        int margin = int(ceilf(blur * 1.5f)) + 4;
        int x0 = std::max(0, int(floor(ix)) - margin), y0 = std::max(0, int(floor(iy)) - margin);
        int x1 = std::min(c->width, int(ceil(ix + iw)) + margin), y1 = std::min(c->height, int(ceil(iy + ih)) + margin);
        if (iw <= 0 || ih <= 0 || x1 <= x0 || y1 <= y0) {
            cairo_surface_destroy(record);
            cairo_surface_flush(c->surface);
            return;
        }
        cairo_surface_t* layer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, x1 - x0, y1 - y0);
        cairo_t* lc = cairo_create(layer);
        cairo_set_source_surface(lc, record, -x0, -y0);
        cairo_paint(lc);
        cairo_destroy(lc);
        cairo_surface_destroy(record);
        cairo_surface_flush(layer);
        cairo_surface_t* mask = shadowMask(layer, blur);
        cairo_save(cr);
        cairo_identity_matrix(cr);
        setSource(cr, c->gs.shadowColor);
        cairo_mask_surface(cr, mask, x0 + c->gs.shadowDx * c->baseScale, y0 - c->gs.shadowDy * c->baseScale);
        cairo_set_source_surface(cr, layer, x0, y0);
        cairo_paint(cr);
        cairo_restore(cr);
        cairo_surface_destroy(mask);
        cairo_surface_destroy(layer);
    }
    cairo_surface_flush(c->surface);
}

void pathOp(CGContext* c, bool fill) {
    cairo_path_t* path = cairo_copy_path(c->cr);
    cairo_new_path(c->cr);
    draw(c, [&](cairo_t* t) {
        cairo_new_path(t);
        cairo_append_path(t, path);
        setSource(t, fill ? c->gs.fill : c->gs.stroke);
        if (fill)
            cairo_fill(t);
        else
            cairo_stroke(t);
    });
    cairo_path_destroy(path);
}

}

CGContext::CGContext() : port::cf::Object(&kContext) {}

namespace port {
namespace gfx {

void showGlyphs(CGContext* c, FontData& font, float size, const float m[4], float x, float y,
                const unsigned short* glyphs, int count) {
    if (!c || count <= 0 || c->gs.textMode == 3) return;
    std::lock_guard<std::recursive_mutex> l(textMutex());
    cairo_font_face_t* face = cairoFace(font);
    if (!face) return;
    std::vector<cairo_glyph_t> run(static_cast<size_t>(count));
    float scale = size / float(unitsPerEm(font));
    float px = x, py = y;
    for (int i = 0; i < count; i++) {
        run[i].index = glyphs[i];
        run[i].x = px;
        run[i].y = py;
        float adv = float(advance(font, glyphs[i])) * scale;
        px += m[0] * adv;
        py += m[1] * adv;
    }
    draw(c, [&](cairo_t* t) {
        cairo_set_font_face(t, face);
        cairo_matrix_t fm;
        cairo_matrix_init(&fm, size * m[0], size * m[1], -size * m[2], -size * m[3], 0, 0);
        cairo_set_font_matrix(t, &fm);
        cairo_font_options_t* o = cairo_font_options_create();
        cairo_font_options_set_hint_style(o, c->gs.antialias ? CAIRO_HINT_STYLE_SLIGHT : CAIRO_HINT_STYLE_NONE);
        cairo_font_options_set_hint_metrics(o, CAIRO_HINT_METRICS_OFF);
        cairo_font_options_set_antialias(o, c->gs.antialias ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE);
        cairo_set_font_options(t, o);
        cairo_font_options_destroy(o);
        int mode = c->gs.textMode;
        if (mode == 0) {
            setSource(t, c->gs.fill);
            cairo_show_glyphs(t, run.data(), count);
        } else {
            cairo_new_path(t);
            cairo_glyph_path(t, run.data(), count);
            if (mode == 2) {
                setSource(t, c->gs.fill);
                cairo_fill_preserve(t);
            }
            setSource(t, c->gs.stroke);
            cairo_stroke(t);
        }
    });
}

}
}

extern "C" {

CGContext* CGBitmapContextCreate(void* data, unsigned long width, unsigned long height, unsigned long bits,
                                 unsigned long bytesPerRow, void*, unsigned int info) {
    if ((info & 0x1f) != 1 || bits != 8)
        fprintf(stderr, "medecomp: CGBitmapContextCreate: формат %u/%lu бит не поддержан, рисую как RGBA\n", info, bits);
    if (width == 0 || height == 0) return nullptr;
    CGContext* c = new CGContext();
    c->width = int(width);
    c->height = int(height);
    int stride = int(bytesPerRow);
    if (stride < c->width * 4 || stride % 4) stride = c->width * 4;
    if (data) {
        c->data = static_cast<uint8_t*>(data);
    } else {
        c->data = new uint8_t[size_t(stride) * c->height]();
        c->ownsData = true;
    }
    c->surface = cairo_image_surface_create_for_data(c->data, CAIRO_FORMAT_ARGB32, c->width, c->height, stride);
    c->cr = cairo_create(c->surface);
    cairo_translate(c->cr, 0, c->height);
    cairo_scale(c->cr, 1, -1);
    return c;
}

CGImageRef CGBitmapContextCreateImage(CGContext* c) {
    if (!c) return nullptr;
    cairo_surface_flush(c->surface);
    CGImage* image = port::gfx::makeImage(c->width, c->height);
    int stride = cairo_image_surface_get_stride(c->surface);
    for (int y = 0; y < c->height; y++)
        memcpy(port::gfx::imagePixels(image) + size_t(y) * c->width * 4, c->data + size_t(y) * stride,
               size_t(c->width) * 4);
    return image;
}

CGContext* CGContextRetain(CGContext* c) {
    CFRetain(c);
    return c;
}

void CGContextRelease(CGContext* c) { CFRelease(c); }

void CGContextSaveGState(CGContext* c) {
    c->saved.push_back(c->gs);
    cairo_save(c->cr);
}

void CGContextRestoreGState(CGContext* c) {
    if (c->saved.empty()) return;
    c->gs = c->saved.back();
    c->saved.pop_back();
    cairo_restore(c->cr);
}

void CGContextTranslateCTM(CGContext* c, float tx, float ty) { cairo_translate(c->cr, tx, ty); }
void CGContextScaleCTM(CGContext* c, float sx, float sy) { cairo_scale(c->cr, sx, sy); }

void medecomp_cg_base_scale(void* context, float scale) {
    CGContext* c = static_cast<CGContext*>(context);
    if (!c || scale <= 0) return;
    cairo_scale(c->cr, scale, scale);
    c->baseScale = scale;
}

void CGContextSetBlendMode(CGContext* c, int mode) { c->gs.blend = mode; }
void CGContextSetInterpolationQuality(CGContext* c, int quality) { c->gs.interpolation = quality; }
void CGContextSetAllowsAntialiasing(CGContext* c, bool allows) { c->gs.antialias = allows; }

void CGContextSetRGBFillColor(CGContext* c, float r, float g, float b, float a) {
    float v[4] = {r, g, b, a};
    memcpy(c->gs.fill, v, sizeof v);
}

void CGContextSetRGBStrokeColor(CGContext* c, float r, float g, float b, float a) {
    float v[4] = {r, g, b, a};
    memcpy(c->gs.stroke, v, sizeof v);
}

void CGContextClipToRect(CGContext* c, CGRect r) {
    cairo_new_path(c->cr);
    cairo_rectangle(c->cr, r.origin.x, r.origin.y, r.size.width, r.size.height);
    cairo_clip(c->cr);
}

void CGContextBeginPath(CGContext* c) { cairo_new_path(c->cr); }
void CGContextMoveToPoint(CGContext* c, float x, float y) { cairo_move_to(c->cr, x, y); }
void CGContextAddLineToPoint(CGContext* c, float x, float y) {
    if (!cairo_has_current_point(c->cr))
        cairo_move_to(c->cr, x, y);
    else
        cairo_line_to(c->cr, x, y);
}
void CGContextAddCurveToPoint(CGContext* c, float x1, float y1, float x2, float y2, float x, float y) {
    if (!cairo_has_current_point(c->cr)) cairo_move_to(c->cr, x1, y1);
    cairo_curve_to(c->cr, x1, y1, x2, y2, x, y);
}
void CGContextFillPath(CGContext* c) { pathOp(c, true); }
void CGContextStrokePath(CGContext* c) { pathOp(c, false); }

void CGContextStrokeRect(CGContext* c, CGRect r) {
    cairo_new_path(c->cr);
    cairo_rectangle(c->cr, r.origin.x, r.origin.y, r.size.width, r.size.height);
    pathOp(c, false);
}

namespace {

cairo_surface_t* upscaleCatmullRom(cairo_surface_t* src, int scale) {
    cairo_surface_flush(src);
    int w = cairo_image_surface_get_width(src), h = cairo_image_surface_get_height(src);
    int ss = cairo_image_surface_get_stride(src);
    const uint8_t* sp = cairo_image_surface_get_data(src);
    int W = w * scale, H = h * scale;
    cairo_surface_t* dst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
    if (cairo_surface_status(dst) != CAIRO_STATUS_SUCCESS || !sp) return dst;
    int ds = cairo_image_surface_get_stride(dst);
    uint8_t* dp = cairo_image_surface_get_data(dst);
    int seen = 0, partial = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t a = sp[y * ss + x * 4 + 3];
            if (a > 16) seen++;
            if (a > 16 && a < 240) partial++;
        }
    bool icon = seen > 0 && partial * 3 < seen;
    auto weights = [](float t, float out[4]) {
        float t2 = t * t, t3 = t2 * t;
        out[0] = -0.5f * t3 + t2 - 0.5f * t;
        out[1] = 1.5f * t3 - 2.5f * t2 + 1.0f;
        out[2] = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        out[3] = 0.5f * t3 - 0.5f * t2;
    };
    std::vector<float> tmp(size_t(W) * h * 4);
    for (int y = 0; y < h; y++)
        for (int X = 0; X < W; X++) {
            float fx = (X + 0.5f) / scale - 0.5f;
            int x0 = int(floorf(fx));
            float wt[4];
            weights(fx - x0, wt);
            float acc[4] = {0, 0, 0, 0};
            for (int i = 0; i < 4; i++) {
                int x = std::min(std::max(x0 - 1 + i, 0), w - 1);
                const uint8_t* px = sp + y * ss + x * 4;
                for (int ch = 0; ch < 4; ch++) acc[ch] += wt[i] * px[ch];
            }
            for (int ch = 0; ch < 4; ch++) tmp[(size_t(y) * W + X) * 4 + ch] = acc[ch];
        }
    for (int Y = 0; Y < H; Y++) {
        float fy = (Y + 0.5f) / scale - 0.5f;
        int y0 = int(floorf(fy));
        float wt[4];
        weights(fy - y0, wt);
        for (int X = 0; X < W; X++) {
            float acc[4] = {0, 0, 0, 0};
            for (int i = 0; i < 4; i++) {
                int y = std::min(std::max(y0 - 1 + i, 0), h - 1);
                const float* px = &tmp[(size_t(y) * W + X) * 4];
                for (int ch = 0; ch < 4; ch++) acc[ch] += wt[i] * px[ch];
            }
            uint8_t* out = dp + Y * ds + X * 4;
            float a = std::min(std::max(acc[3], 0.0f), 255.0f);
            float k = 1.0f;
            if (icon && a > 0.5f) {
                float t = std::min(std::max((a / 255.0f - 0.3f) / 0.4f, 0.0f), 1.0f);
                float sharp = 255.0f * t * t * (3.0f - 2.0f * t);
                k = sharp / a;
                a = sharp;
            }
            out[3] = uint8_t(a + 0.5f);
            for (int ch = 0; ch < 3; ch++) out[ch] = uint8_t(std::min(std::max(acc[ch] * k, 0.0f), a) + 0.5f);
        }
    }
    cairo_surface_mark_dirty(dst);
    return dst;
}

cairo_user_data_key_t s_upscaledKey;

struct Upscaled {
    int scale;
    cairo_surface_t* surface;
};

struct VectorStar {
    double cx, cy, radius;
    bool pointUp;
    double r, g, b, a;
};

cairo_user_data_key_t s_starsKey;

struct Stars {
    std::vector<VectorStar> stars;
};

std::vector<VectorStar> findStars(cairo_surface_t* src) {
    std::vector<VectorStar> out;
    cairo_surface_flush(src);
    int w = cairo_image_surface_get_width(src), h = cairo_image_surface_get_height(src);
    int ss = cairo_image_surface_get_stride(src);
    const uint8_t* sp = cairo_image_surface_get_data(src);
    if (!sp || w != 73 || h != 18 || cairo_image_surface_get_format(src) != CAIRO_FORMAT_ARGB32) return out;
    auto alpha = [&](int x, int y) { return sp[y * ss + x * 4 + 3]; };
    std::vector<int> label(size_t(w) * h, -1);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            if (alpha(x, y) <= 16 || label[size_t(y) * w + x] >= 0) continue;
            int id = int(out.size());
            std::vector<std::pair<int, int>> stack{{x, y}}, pixels;
            label[size_t(y) * w + x] = id;
            while (!stack.empty()) {
                auto [px, py] = stack.back();
                stack.pop_back();
                pixels.push_back({px, py});
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = px + dx, ny = py + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h || alpha(nx, ny) <= 16) continue;
                        if (label[size_t(ny) * w + nx] >= 0) continue;
                        label[size_t(ny) * w + nx] = id;
                        stack.push_back({nx, ny});
                    }
            }
            double mass = 0, mx = 0, my = 0, cr = 0, cg = 0, cb = 0;
            int minY = h, maxY = 0;
            for (auto [px, py] : pixels) {
                const uint8_t* q = sp + py * ss + px * 4;
                double a = q[3] / 255.0;
                mass += a;
                mx += a * (px + 0.5);
                my += a * (py + 0.5);
                cb += q[0], cg += q[1], cr += q[2];
                minY = std::min(minY, py), maxY = std::max(maxY, py);
            }
            if (mass < 4) return {};
            VectorStar star;
            star.cx = mx / mass;
            star.cy = my / mass;
            double radius = 0;
            for (auto [px, py] : pixels)
                if (sp[py * ss + px * 4 + 3] >= 128)
                    radius = std::max(radius, std::hypot(px + 0.5 - star.cx, py + 0.5 - star.cy));
            star.radius = radius + 0.6;
            int brightest = 0;
            for (auto [px, py] : pixels) {
                const uint8_t* q = sp + py * ss + px * 4;
                if (q[3] >= 240) brightest = std::max(brightest, q[0] + q[1] + q[2]);
            }
            double n = 0;
            cr = cg = cb = 0;
            for (auto [px, py] : pixels) {
                const uint8_t* q = sp + py * ss + px * 4;
                if (q[3] < 240 || (q[0] + q[1] + q[2]) * 5 < brightest * 4) continue;
                cb += q[0], cg += q[1], cr += q[2];
                n++;
            }
            if (n == 0) return {};
            star.r = cr / n / 255.0, star.g = cg / n / 255.0, star.b = cb / n / 255.0;
            star.a = 1.0;
            auto runs = [&](int row) {
                int n = 0;
                bool in = false;
                for (int x = 0; x < w; x++) {
                    bool on = label[size_t(row) * w + x] == id && alpha(x, row) >= 128;
                    if (on && !in) n++;
                    in = on;
                }
                return n;
            };
            star.pointUp = runs(minY + 1) < runs(maxY - 1);
            out.push_back(star);
        }
    if (out.size() != 3) out.clear();
    return out;
}

const std::vector<VectorStar>& starsOf(cairo_surface_t* src) {
    Stars* found = static_cast<Stars*>(cairo_surface_get_user_data(src, &s_starsKey));
    if (!found) {
        found = new Stars;
        found->stars = findStars(src);
        cairo_surface_set_user_data(src, &s_starsKey, found, [](void* p) { delete static_cast<Stars*>(p); });
    }
    return found->stars;
}

void drawStars(cairo_t* t, const std::vector<VectorStar>& stars) {
    for (const VectorStar& star : stars) {
        cairo_new_path(t);
        double dir = star.pointUp ? -1 : 1;
        for (int i = 0; i < 10; i++) {
            double radius = i % 2 ? star.radius * 0.382 : star.radius;
            double angle = M_PI / 5 * i;
            double x = star.cx + radius * sin(angle), y = star.cy + dir * radius * cos(angle);
            i ? cairo_line_to(t, x, y) : cairo_move_to(t, x, y);
        }
        cairo_close_path(t);
        cairo_set_source_rgba(t, star.r, star.g, star.b, star.a);
        cairo_fill(t);
    }
}

cairo_surface_t* upscaledFor(cairo_surface_t* src, int scale) {
    Upscaled* u = static_cast<Upscaled*>(cairo_surface_get_user_data(src, &s_upscaledKey));
    if (u && u->scale == scale) return u->surface;
    Upscaled* made = new Upscaled{scale, upscaleCatmullRom(src, scale)};
    cairo_surface_set_user_data(src, &s_upscaledKey, made, [](void* p) {
        Upscaled* v = static_cast<Upscaled*>(p);
        cairo_surface_destroy(v->surface);
        delete v;
    });
    return made->surface;
}

}

void CGContextDrawImage(CGContext* c, CGRect r, CGImageRef image) {
    if (!c || !image || r.size.width == 0 || r.size.height == 0) return;
    cairo_surface_t* s = port::gfx::imageSurface(image);
    if (!s) return;
    int iw = port::gfx::imageWidth(image), ih = port::gfx::imageHeight(image);
    draw(c, [&](cairo_t* t) {
        cairo_save(t);
        float magnify = c->baseScale * fabsf(r.size.width) / iw;
        int k = magnify >= 1.5f ? std::min(8, int(magnify + 0.5f)) : 1;
        if (k > 1) {
            const std::vector<VectorStar>& stars = starsOf(s);
            if (!stars.empty()) {
                cairo_translate(t, r.origin.x, r.origin.y + r.size.height);
                cairo_scale(t, r.size.width / iw, -r.size.height / ih);
                drawStars(t, stars);
                cairo_restore(t);
                return;
            }
        }
        bool upscale = k > 1 && iw * ih <= 512 * 512;
        cairo_surface_t* source = upscale ? upscaledFor(s, k) : s;
        int sw = upscale ? iw * k : iw, sh = upscale ? ih * k : ih;
        cairo_translate(t, r.origin.x, r.origin.y + r.size.height);
        cairo_scale(t, r.size.width / sw, -r.size.height / sh);
        cairo_set_source_surface(t, source, 0, 0);
        cairo_pattern_t* pattern = cairo_get_source(t);
        bool nearest = c->gs.interpolation == 1 && (c->baseScale <= 1 || (k == 1 && magnify < 1.01f));
        cairo_pattern_set_filter(pattern, nearest ? CAIRO_FILTER_NEAREST : CAIRO_FILTER_BILINEAR);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
        cairo_new_path(t);
        cairo_rectangle(t, 0, 0, sw, sh);
        cairo_fill(t);
        cairo_restore(t);
    });
}

void CGContextSetShadowWithColor(CGContext* c, CGSize offset, float blur, void* color) {
    c->gs.shadow = color != nullptr;
    c->gs.shadowDx = offset.width;
    c->gs.shadowDy = offset.height;
    c->gs.shadowBlur = blur;
    if (color) port::gfx::colorComponents(color, c->gs.shadowColor);
}

void CGContextSetTextDrawingMode(CGContext* c, int mode) { c->gs.textMode = mode; }

void CGContextSetFont(CGContext* c, CGFont* font) {
    if (font) {
        CFRetain(font);
        c->fonts.push_back(font);
    }
    c->gs.font = font;
}

void CGContextSetFontSize(CGContext* c, float size) { c->gs.fontSize = size; }

void CGContextSetTextMatrix(CGContext* c, float a, float b, float cc, float d, float, float) {
    float m[4] = {a, b, cc, d};
    memcpy(c->gs.text, m, sizeof m);
}

void CGContextShowGlyphsAtPoint(CGContext* c, float x, float y, const unsigned short* glyphs, int count) {
    port::gfx::FontData* f = port::gfx::fontOf(c->gs.font);
    if (f) port::gfx::showGlyphs(c, *f, c->gs.fontSize, c->gs.text, x, y, glyphs, count);
}

}
