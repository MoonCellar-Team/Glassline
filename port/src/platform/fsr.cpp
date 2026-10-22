#include "fsr.h"

namespace port {
namespace platform {

const char* const kFsrEasuFragment = R"(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
uniform sampler2D u_tex;
uniform vec4 u_con0;
uniform vec2 u_texel;

float luma(vec3 c) { return c.b * 0.5 + (c.r * 0.5 + c.g); }

void setF(inout vec2 dir, inout float len, float w, float lA, float lB, float lC, float lD, float lE) {
    float dc = lD - lC;
    float cb = lC - lB;
    float lenX = max(abs(dc), abs(cb));
    lenX = lenX > 0.0 ? 1.0 / lenX : 0.0;
    float dirX = lD - lB;
    dir.x += dirX * w;
    lenX = clamp(abs(dirX) * lenX, 0.0, 1.0);
    lenX *= lenX;
    len += lenX * w;
    float ec = lE - lC;
    float ca = lC - lA;
    float lenY = max(abs(ec), abs(ca));
    lenY = lenY > 0.0 ? 1.0 / lenY : 0.0;
    float dirY = lE - lA;
    dir.y += dirY * w;
    lenY = clamp(abs(dirY) * lenY, 0.0, 1.0);
    lenY *= lenY;
    len += lenY * w;
}

void tapF(inout vec3 aC, inout float aW, vec2 off, vec2 dir, vec2 len2, float lob, float clp, vec3 c) {
    vec2 v = vec2(off.x * dir.x + off.y * dir.y, off.x * (-dir.y) + off.y * dir.x) * len2;
    float d2 = min(v.x * v.x + v.y * v.y, clp);
    float wB = 2.0 / 5.0 * d2 - 1.0;
    float wA = lob * d2 - 1.0;
    wB *= wB;
    wA *= wA;
    wB = 25.0 / 16.0 * wB - (25.0 / 16.0 - 1.0);
    float w = wB * wA;
    aC += c * w;
    aW += w;
}

vec3 at(vec2 fp, float x, float y) { return texture2D(u_tex, (fp + vec2(x, y) + 0.5) * u_texel).rgb; }

void main() {
    vec2 pp = floor(gl_FragCoord.xy) * u_con0.xy + u_con0.zw;
    vec2 fp = floor(pp);
    pp -= fp;
    vec3 b = at(fp, 0.0, -1.0), c = at(fp, 1.0, -1.0);
    vec3 e = at(fp, -1.0, 0.0), f = at(fp, 0.0, 0.0), g = at(fp, 1.0, 0.0), h = at(fp, 2.0, 0.0);
    vec3 i = at(fp, -1.0, 1.0), j = at(fp, 0.0, 1.0), k = at(fp, 1.0, 1.0), l = at(fp, 2.0, 1.0);
    vec3 n = at(fp, 0.0, 2.0), o = at(fp, 1.0, 2.0);
    float bL = luma(b), cL = luma(c), eL = luma(e), fL = luma(f), gL = luma(g), hL = luma(h);
    float iL = luma(i), jL = luma(j), kL = luma(k), lL = luma(l), nL = luma(n), oL = luma(o);

    vec2 dir = vec2(0.0);
    float len = 0.0;
    setF(dir, len, (1.0 - pp.x) * (1.0 - pp.y), bL, eL, fL, gL, jL);
    setF(dir, len, pp.x * (1.0 - pp.y), cL, fL, gL, hL, kL);
    setF(dir, len, (1.0 - pp.x) * pp.y, fL, iL, jL, kL, nL);
    setF(dir, len, pp.x * pp.y, gL, jL, kL, lL, oL);

    float dirR = dir.x * dir.x + dir.y * dir.y;
    bool zro = dirR < 1.0 / 32768.0;
    dirR = zro ? 1.0 : inversesqrt(dirR);
    dir.x = zro ? 1.0 : dir.x;
    dir *= dirR;
    len = len * 0.5;
    len *= len;
    float stretch = (dir.x * dir.x + dir.y * dir.y) / max(abs(dir.x), abs(dir.y));
    vec2 len2 = vec2(1.0 + (stretch - 1.0) * len, 1.0 - 0.5 * len);
    float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
    float clp = 1.0 / lob;

    vec3 aC = vec3(0.0);
    float aW = 0.0;
    tapF(aC, aW, vec2(0.0, -1.0) - pp, dir, len2, lob, clp, b);
    tapF(aC, aW, vec2(1.0, -1.0) - pp, dir, len2, lob, clp, c);
    tapF(aC, aW, vec2(-1.0, 1.0) - pp, dir, len2, lob, clp, i);
    tapF(aC, aW, vec2(0.0, 1.0) - pp, dir, len2, lob, clp, j);
    tapF(aC, aW, vec2(0.0, 0.0) - pp, dir, len2, lob, clp, f);
    tapF(aC, aW, vec2(-1.0, 0.0) - pp, dir, len2, lob, clp, e);
    tapF(aC, aW, vec2(1.0, 1.0) - pp, dir, len2, lob, clp, k);
    tapF(aC, aW, vec2(2.0, 1.0) - pp, dir, len2, lob, clp, l);
    tapF(aC, aW, vec2(2.0, 0.0) - pp, dir, len2, lob, clp, h);
    tapF(aC, aW, vec2(1.0, 0.0) - pp, dir, len2, lob, clp, g);
    tapF(aC, aW, vec2(1.0, 2.0) - pp, dir, len2, lob, clp, o);
    tapF(aC, aW, vec2(0.0, 2.0) - pp, dir, len2, lob, clp, n);

    vec3 mn = min(min(f, g), min(j, k));
    vec3 mx = max(max(f, g), max(j, k));
    vec3 pix = aW != 0.0 ? aC / aW : f;
    gl_FragColor = vec4(min(mx, max(mn, pix)), 1.0);
}
)";

const char* const kFsrRcasFragment = R"(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform float u_sharp;
varying vec2 v_uv;

void main() {
    vec3 b = texture2D(u_tex, v_uv + vec2(0.0, -u_texel.y)).rgb;
    vec3 d = texture2D(u_tex, v_uv + vec2(-u_texel.x, 0.0)).rgb;
    vec3 e = texture2D(u_tex, v_uv).rgb;
    vec3 f = texture2D(u_tex, v_uv + vec2(u_texel.x, 0.0)).rgb;
    vec3 h = texture2D(u_tex, v_uv + vec2(0.0, u_texel.y)).rgb;
    vec3 mn4 = min(min(b, d), min(f, h));
    vec3 mx4 = max(max(b, d), max(f, h));
    // В ffx_fsr1.h делители без защиты: на чисто чёрном и белом в GLSL получился бы NaN
    vec3 hitMin = min(mn4, e) / max(4.0 * mx4, vec3(1.0 / 65536.0));
    vec3 hitMax = (1.0 - max(mx4, e)) / min(4.0 * mn4 - 4.0, vec3(-1.0 / 65536.0));
    vec3 lobeRGB = max(-hitMin, hitMax);
    float lobe = max(-(0.25 - 1.0 / 16.0), min(max(lobeRGB.r, max(lobeRGB.g, lobeRGB.b)), 0.0)) * u_sharp;
    vec3 pix = (lobe * (b + d + f + h) + e) / (4.0 * lobe + 1.0);
    gl_FragColor = vec4(pix, 1.0);
}
)";

}
}
