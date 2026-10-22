#pragma once

#if defined(MEDECOMP_PORT) && defined(_WIN32)
#include "port/win_compat.h"
#endif

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <endian.h>
#undef BIG_ENDIAN
#undef LITTLE_ENDIAN
#undef PDP_ENDIAN
#undef BYTE_ORDER

#ifdef __cplusplus
inline int isnumber(int c) { return isdigit(c); }
#endif

#ifdef __cplusplus
extern "C" int medecomp_wide_extra(void);
inline float medecomp_wide_fov(float fov) {
    int extra = medecomp_wide_extra();
    if (extra == 0) return fov;
    return 2.0f * atanf(tanf(fov * 0.00872664626f) * (1024.0f + 2.0f * extra) / 1024.0f) * 57.2957795f;
}
inline float medecomp_wide_aspect(float height) { return height / (1024.0f + 2.0f * medecomp_wide_extra()); }
inline float medecomp_hor_fov(float vfov, float width, float height) {
    return 2.0f * atanf(tanf(vfov * 0.00872664626f) * width / height) * 57.2957795f;
}
#endif
#if defined(MEDECOMP_PORT) && !defined(__ANDROID__)
#define MEDECOMP_SPLIT_STACKED 1
#endif
#ifdef __cplusplus
extern "C" float medecomp_render_scale(void);
inline int medecomp_px(int units) { return (int)lroundf((float)units * medecomp_render_scale()); }
inline int medecomp_text_scale(void) {
    float s = medecomp_render_scale();
    return s <= 1.0f ? 1 : s <= 2.0f ? 2 : 4;
}
extern "C" const char* medecomp_language(void);
extern "C" void medecomp_language_changed(const void* cfLocale);
extern "C" void medecomp_cg_base_scale(void* context, float scale);
namespace m3g {
class RenderNode;
class Renderer;
class Transform;
}
namespace midp {
class GLES20;
}
const unsigned MEDECOMP_SHADOW_FLAG = 0x10000000u;
void medecomp_shadow_pass(const m3g::RenderNode* nodes, int count, m3g::Renderer* renderer);
void medecomp_shadow_end(void);
bool medecomp_shadows_active(void);
bool medecomp_shadows_enabled(void);
void medecomp_shadow_receive(midp::GLES20* gl, int program, const m3g::Transform* model);
bool medecomp_taa_jitter(float out[2]);
extern const char* const medecomp_shadow_vertex_header;
extern const char* const medecomp_shadow_vertex_footer;
extern const char* const medecomp_shadow_pixel_header;
extern const char* const medecomp_shadow_pixel_after;
#endif
