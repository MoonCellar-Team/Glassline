#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "../gfx/png.h"
#include "../gfx/attribs.h"
#include "gl_shared.h"
#include "m3g/Transform.h"
#include "port/files.h"
#include "port/platform.h"
#include "settings.h"

namespace port {
namespace platform {

namespace {

const char* kVertex =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";

const char* kBright =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_texel;\n"
    "uniform float u_threshold;\n"
    "void main() {\n"
    "  vec3 c = (texture2D(u_tex, v_uv + u_texel * vec2(-0.5, -0.5)).rgb +\n"
    "            texture2D(u_tex, v_uv + u_texel * vec2(0.5, -0.5)).rgb +\n"
    "            texture2D(u_tex, v_uv + u_texel * vec2(-0.5, 0.5)).rgb +\n"
    "            texture2D(u_tex, v_uv + u_texel * vec2(0.5, 0.5)).rgb) * 0.25;\n"
    "  float l = max(c.r, max(c.g, c.b));\n"
    "  float k = clamp((l - u_threshold) / (1.0 - u_threshold), 0.0, 1.0);\n"
    "  gl_FragColor = vec4(c * k * k, 1.0);\n"
    "}\n";

const char* kDown =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_texel;\n"
    "void main() {\n"
    "  gl_FragColor = (texture2D(u_tex, v_uv + u_texel * vec2(-0.5, -0.5)) +\n"
    "                  texture2D(u_tex, v_uv + u_texel * vec2(0.5, -0.5)) +\n"
    "                  texture2D(u_tex, v_uv + u_texel * vec2(-0.5, 0.5)) +\n"
    "                  texture2D(u_tex, v_uv + u_texel * vec2(0.5, 0.5))) * 0.25;\n"
    "}\n";

const char* kBlur =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_dir;\n"
    "void main() {\n"
    "  vec4 c = texture2D(u_tex, v_uv) * 0.227027;\n"
    "  c += (texture2D(u_tex, v_uv + u_dir * 1.384615) + texture2D(u_tex, v_uv - u_dir * 1.384615)) * 0.316216;\n"
    "  c += (texture2D(u_tex, v_uv + u_dir * 3.230769) + texture2D(u_tex, v_uv - u_dir * 3.230769)) * 0.070270;\n"
    "  gl_FragColor = c;\n"
    "}\n";

const char* kAoCommon =
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_depth;\n"
    "uniform vec2 u_texel;\n"
    "uniform vec4 u_proj;\n"
    "uniform float u_radius;\n"
    "vec3 viewPos(vec2 uv) {\n"
    "  float d = texture2D(u_depth, uv).r * 2.0 - 1.0;\n"
    "  float z = 2.0 * u_proj.z * u_proj.w / ((u_proj.w - u_proj.z) * d - (u_proj.w + u_proj.z));\n"
    "  return vec3((uv * 2.0 - 1.0) * u_proj.xy * -z, z);\n"
    "}\n"
    "vec3 viewNormal(vec2 uv, vec3 p) {\n"
    "  vec3 px = viewPos(uv + vec2(u_texel.x, 0.0)) - p, nx = p - viewPos(uv - vec2(u_texel.x, 0.0));\n"
    "  vec3 py = viewPos(uv + vec2(0.0, u_texel.y)) - p, ny = p - viewPos(uv - vec2(0.0, u_texel.y));\n"
    "  vec3 n = normalize(cross(abs(px.z) < abs(nx.z) ? px : nx, abs(py.z) < abs(ny.z) ? py : ny));\n"
    "  return dot(n, p) > 0.0 ? -n : n;\n"
    "}\n";

const char* kAo =
    "void main() {\n"
    "  if (texture2D(u_depth, v_uv).r > 0.99999) { gl_FragColor = vec4(1.0); return; }\n"
    "  vec3 p = viewPos(v_uv);\n"
    "  vec3 n = viewNormal(v_uv, p);\n"
    "  vec2 r = u_radius / (2.0 * -p.z * u_proj.xy);\n"
    "  r = min(r, u_texel * 96.0);\n"
    "  vec2 cell = mod(floor(gl_FragCoord.xy), 4.0);\n"
    "  float a0 = 6.2831853 * (cell.x * 4.0 + cell.y) / 16.0;\n"
    "  float occ = 0.0;\n"
    "  for (int i = 0; i < 12; i++) {\n"
    "    float t = (float(i) + 0.5) / 12.0;\n"
    "    float a = a0 + float(i) * 2.3999632;\n"
    "    vec3 v = viewPos(v_uv + vec2(cos(a), sin(a)) * sqrt(t) * r) - p;\n"
    "    float vv = dot(v, v);\n"
    "    float falloff = max(0.0, 1.0 - vv / (u_radius * u_radius));\n"
    "    occ += max(0.0, dot(v, n) * inversesqrt(vv + 1e-6) - 0.3) * falloff;\n"
    "  }\n"
    "  gl_FragColor = vec4(clamp(1.0 - occ / 12.0 * 2.5, 0.0, 1.0), 1.0, 1.0, 1.0);\n"
    "}\n";

const char* kHorizon =
    "void main() {\n"
    "  vec2 px = floor(gl_FragCoord.xy);\n"
    "  vec2 uv = (px * 2.0 + 0.5) * u_texel;\n"
    "  if (texture2D(u_depth, uv).r > 0.99999) { gl_FragColor = vec4(1.0); return; }\n"
    "  vec3 p = viewPos(uv);\n"
    "  vec3 n = viewNormal(uv, p);\n"
    "  vec2 r = min(u_radius / (2.0 * -p.z * u_proj.xy), u_texel * 96.0);\n"
    "  float minS = 1.5 * u_texel.x / r.x;\n"
    "  vec2 c = mod(px, 4.0), lo = mod(c, 2.0), hi = floor(c / 2.0);\n"
    "  float noise = (mod(2.0 * lo.x + 3.0 * lo.y, 4.0) * 4.0 + mod(2.0 * hi.x + 3.0 * hi.y, 4.0) + 0.5) / 16.0;\n"
    "  float jitter = fract(noise * 7.0);\n"
    "  float R2 = u_radius * u_radius;\n"
    "#ifndef GTAO\n"
    "  float ao = 0.0;\n"
    "  for (int d = 0; d < 8; d++) {\n"
    "    float a = (float(d) + noise) * 0.7853982;\n"
    "    vec2 dir = vec2(cos(a), sin(a)) * r;\n"
    "    float top = 0.2, occ = 0.0;\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "      float s = (float(i) + jitter) / 4.0;\n"
    "      vec3 v = viewPos(uv + dir * (s * s + minS)) - p;\n"
    "      float vv = dot(v, v);\n"
    "      float h = dot(v, n) * inversesqrt(vv + 1e-6);\n"
    "      if (h > top) {\n"
    "        occ += (h - top) * max(0.0, 1.0 - vv / R2);\n"
    "        top = h;\n"
    "      }\n"
    "    }\n"
    "    ao += occ;\n"
    "  }\n"
    "  gl_FragColor = vec4(clamp(1.0 - ao / 8.0 * 2.0, 0.0, 1.0), 1.0, 1.0, 1.0);\n"
    "#else\n"
    "  vec3 view = normalize(-p);\n"
    "  float vis = 0.0;\n"
    "  for (int d = 0; d < 4; d++) {\n"
    "    float a = (float(d) + noise) * 0.7853982;\n"
    "    vec2 o = vec2(cos(a), sin(a));\n"
    "    vec3 dir = vec3(o, 0.0);\n"
    "    vec3 ortho = dir - dot(dir, view) * view;\n"
    "    vec3 axis = normalize(cross(ortho, view));\n"
    "    vec3 pn = n - axis * dot(n, axis);\n"
    "    float pl = max(length(pn), 1e-4);\n"
    "    float cosN = clamp(dot(pn, view) / pl, 0.0, 1.0);\n"
    "    float na = sign(dot(ortho, pn)) * acos(cosN);\n"
    "    float low0 = cos(na + 1.5707963), low1 = cos(na - 1.5707963);\n"
    "    float c0 = low0, c1 = low1;\n"
    "    for (int i = 0; i < 4; i++) {\n"
    "      float s = (float(i) + jitter) / 4.0;\n"
    "      vec2 off = o * r * (s * s + minS);\n"
    "      vec3 v0 = viewPos(uv + off) - p, v1 = viewPos(uv - off) - p;\n"
    "      float l0 = max(length(v0), 1e-4), l1 = max(length(v1), 1e-4);\n"
    "      c0 = max(c0, mix(low0, dot(v0, view) / l0, clamp((u_radius - l0) / (0.6 * u_radius), 0.0, 1.0)));\n"
    "      c1 = max(c1, mix(low1, dot(v1, view) / l1, clamp((u_radius - l1) / (0.6 * u_radius), 0.0, 1.0)));\n"
    "    }\n"
    "    float h0 = na + max(-acos(clamp(c1, -1.0, 1.0)) - na, -1.5707963);\n"
    "    float h1 = na + min(acos(clamp(c0, -1.0, 1.0)) - na, 1.5707963);\n"
    "    vis += pl * (2.0 * cosN + 2.0 * (h0 + h1) * sin(na) - cos(2.0 * h0 - na) - cos(2.0 * h1 - na)) * 0.25;\n"
    "  }\n"
    "  vis = clamp(vis / 4.0, 0.0, 1.0);\n"
    "  gl_FragColor = vec4(vis * vis, 1.0, 1.0, 1.0);\n"
    "#endif\n"
    "}\n";

const char* kAoBlur =
    "precision highp float;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_depth;\n"
    "uniform vec2 u_dir;\n"
    "uniform vec2 u_aoTexel;\n"
    "uniform vec2 u_texel;\n"
    "uniform float u_scale;\n"
    "uniform vec4 u_proj;\n"
    "float dist(vec2 px) {\n"
    "  float d = texture2D(u_depth, (px * u_scale + 0.5) * u_texel).r * 2.0 - 1.0;\n"
    "  return 2.0 * u_proj.z * u_proj.w / ((u_proj.w + u_proj.z) - (u_proj.w - u_proj.z) * d);\n"
    "}\n"
    "void main() {\n"
    "  vec2 px = floor(gl_FragCoord.xy);\n"
    "  float zc = dist(px);\n"
    "  float sum = 0.0, wsum = 0.0;\n"
    "  for (int i = -4; i <= 4; i++) {\n"
    "    vec2 q = px + u_dir * float(i);\n"
    "    float w = exp(-float(i * i) / 18.0) * max(0.0, 1.0 - abs(dist(q) - zc) / (0.03 * zc));\n"
    "    sum += texture2D(u_tex, (q + 0.5) * u_aoTexel).r * w;\n"
    "    wsum += w;\n"
    "  }\n"
    "  gl_FragColor = vec4(sum / max(wsum, 1e-4), 1.0, 1.0, 1.0);\n"
    "}\n";

const char* kAoUp =
    "precision highp float;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_depth;\n"
    "uniform vec2 u_aoTexel;\n"
    "uniform vec2 u_texel;\n"
    "uniform vec4 u_proj;\n"
    "float dist(vec2 uv) {\n"
    "  float d = texture2D(u_depth, uv).r * 2.0 - 1.0;\n"
    "  return 2.0 * u_proj.z * u_proj.w / ((u_proj.w + u_proj.z) - (u_proj.w - u_proj.z) * d);\n"
    "}\n"
    "void main() {\n"
    "  vec2 px = floor(gl_FragCoord.xy);\n"
    "  float zc = dist((px + 0.5) * u_texel);\n"
    "  vec2 h = px * 0.5 - 0.25, base = floor(h), f = h - base;\n"
    "  float sum = 0.0, wsum = 0.0, best = 1.0, bestDz = 1e9;\n"
    "  for (int i = 0; i < 4; i++) {\n"
    "    vec2 o = vec2(mod(float(i), 2.0), floor(float(i) / 2.0));\n"
    "    vec2 q = max(base + o, 0.0);\n"
    "    float dz = abs(dist((q * 2.0 + 0.5) * u_texel) - zc);\n"
    "    float ao = texture2D(u_tex, (q + 0.5) * u_aoTexel).r;\n"
    "    vec2 b = mix(1.0 - f, f, o);\n"
    "    float w = b.x * b.y * max(0.0, 1.0 - dz / (0.03 * zc));\n"
    "    sum += ao * w;\n"
    "    wsum += w;\n"
    "    if (dz < bestDz) {\n"
    "      bestDz = dz;\n"
    "      best = ao;\n"
    "    }\n"
    "  }\n"
    "  gl_FragColor = vec4(wsum > 1e-3 ? sum / wsum : best, 1.0, 1.0, 1.0);\n"
    "}\n";

const char* kRaysMask =
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_depth;\n"
    "uniform vec2 u_sun;\n"
    "uniform vec2 u_aspect;\n"
    "void main() {\n"
    "  vec3 c = texture2D(u_tex, v_uv).rgb;\n"
    "  float sky = step(0.99999, texture2D(u_depth, v_uv).r);\n"
    "  float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "  float glow = 1.0 - smoothstep(0.0, 0.5, length((v_uv - u_sun) * u_aspect));\n"
    "  gl_FragColor = vec4(c * sky * smoothstep(0.5, 0.95, l) * glow, 1.0);\n"
    "}\n";

const char* kRays =
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_sun;\n"
    "void main() {\n"
    "  vec2 step = (u_sun - v_uv) * (0.9 / 48.0);\n"
    "  float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n"
    "  vec2 uv = v_uv + step * noise;\n"
    "  vec3 sum = vec3(0.0);\n"
    "  float w = 1.0;\n"
    "  for (int i = 0; i < 48; i++) {\n"
    "    sum += texture2D(u_tex, uv).rgb * w;\n"
    "    w *= 0.96;\n"
    "    uv += step;\n"
    "  }\n"
    "  gl_FragColor = vec4(sum * (1.6 / 21.5), 1.0);\n"
    "}\n";

const char* kSsr =
    "uniform sampler2D u_tex;\n"
    "uniform vec4 u_up;\n"
    "uniform float u_strength;\n"
    "vec2 toUV(vec3 q) { return q.xy / (-q.z * u_proj.xy) * 0.5 + 0.5; }\n"
    "vec2 cell(vec2 uv) {\n"
    "  return (floor(clamp(uv, 0.0, 0.9999) / u_texel) + 0.5) * u_texel;\n"
    "}\n"
    "bool raised(vec3 p, vec2 uv, vec3 q, float thick) {\n"
    "  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return false;\n"
    "  if (texture2D(u_depth, uv).r > 0.99999) return false;\n"
    "  vec3 hs = viewPos(uv);\n"
    "  if (!(q.z < hs.z && hs.z - q.z < thick)) return false;\n"
    "  if (hs.z > p.z) return false;\n"
    "  if (dot(hs - p, u_up.xyz) <= 0.025 && dot(hs - p, hs - p) < 36.0) return false;\n"
    "  return true;\n"
    "}\n"
    "bool skyHole(vec3 p, vec2 uv) {\n"
    "  float opp = 0.0;\n"
    "  for (int ring = 0; ring < 2; ring++) {\n"
    "    float rad = ring == 0 ? 10.0 : 28.0;\n"
    "    float c0 = 0.0, c1 = 0.0, c2 = 0.0, c3 = 0.0, c4 = 0.0, c5 = 0.0, c6 = 0.0, c7 = 0.0;\n"
    "    for (int i = 0; i < 8; i++) {\n"
    "      float a = float(i) * 0.78539816;\n"
    "      vec2 q = uv + vec2(cos(a), sin(a)) * u_texel * rad;\n"
    "      float near = 0.0;\n"
    "      if (q.x >= 0.0 && q.y >= 0.0 && q.x <= 1.0 && q.y <= 1.0 && texture2D(u_depth, q).r <= 0.99999) {\n"
    "        vec3 hs = viewPos(q);\n"
    "        if (dot(hs - p, hs - p) < 36.0) near = 1.0;\n"
    "      }\n"
    "      if (i == 0) c0 = near;\n"
    "      else if (i == 1) c1 = near;\n"
    "      else if (i == 2) c2 = near;\n"
    "      else if (i == 3) c3 = near;\n"
    "      else if (i == 4) c4 = near;\n"
    "      else if (i == 5) c5 = near;\n"
    "      else if (i == 6) c6 = near;\n"
    "      else c7 = near;\n"
    "    }\n"
    "    opp += c0 * c4 + c1 * c5 + c2 * c6 + c3 * c7;\n"
    "  }\n"
    "  return opp > 0.0;\n"
    "}\n"
    "vec2 onWall(vec3 p, vec3 r, vec2 hit) {\n"
    "  vec3 hs = viewPos(hit);\n"
    "  vec3 hn = viewNormal(hit, hs);\n"
    "  if (abs(dot(hn, u_up.xyz)) > 0.45) return hit;\n"
    "  float denom = dot(r, hn);\n"
    "  if (abs(denom) < 0.05) return hit;\n"
    "  float tp = dot(hs - p, hn) / denom;\n"
    "  if (!(tp > 0.02)) return hit;\n"
    "  vec2 uv = toUV(p + r * tp);\n"
    "  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return hit;\n"
    "  if (texture2D(u_depth, uv).r > 0.99999) return hit;\n"
    "  vec3 hs2 = viewPos(uv);\n"
    "  if (abs(hs2.z - (p + r * tp).z) > 1.25) return hit;\n"
    "  if (dot(hn, viewNormal(uv, hs2)) < 0.7) return hit;\n"
    "  vec2 d = (uv - hit) / u_texel;\n"
    "  if (dot(d, d) > 160.0 * 160.0) return hit;\n"
    "  return cell(uv);\n"
    "}\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0);\n"
    "  if (texture2D(u_depth, v_uv).r > 0.99999) return;\n"
    "  vec3 p = viewPos(v_uv);\n"
    "  vec3 n = viewNormal(v_uv, p);\n"
    "  float facing = dot(n, u_up.xyz);\n"
    "  float level = smoothstep(0.9, 0.97, facing);\n"
    "  if (level <= 0.0) return;\n"
    "  n = u_up.xyz;\n"
    "  vec3 v = normalize(p);\n"
    "  vec3 r = reflect(v, n);\n"
    "  if (r.z > 0.0) return;\n"
    "  float t = 0.0, prev = 0.0;\n"
    "  vec2 hit = vec2(-1.0), last = v_uv, skyAt = vec2(-1.0);\n"
    "  for (int i = 0; i < 48; i++) {\n"
    "    float stride = i < 16 ? 1.0 : (i < 24 ? 2.0 : (i < 32 ? 4.0 : (i < 40 ? 8.0 : 16.0)));\n"
    "    float eps = 0.05;\n"
    "    float base = t + eps;\n"
    "    vec2 ahead = toUV(p + r * (base + eps)) - toUV(p + r * base);\n"
    "    float pix = length(ahead / u_texel);\n"
    "    float dt = stride * eps / max(pix, 0.02);\n"
    "    if (dt < 0.02) dt = 0.02;\n"
    "    if (dt > 10.0) dt = 10.0;\n"
    "    prev = t;\n"
    "    t += dt;\n"
    "    vec3 q = p + r * t;\n"
    "    vec2 uv = toUV(q);\n"
    "    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;\n"
    "    last = uv;\n"
    "    if (skyAt.x < 0.0 && texture2D(u_depth, uv).r > 0.99999) skyAt = uv;\n"
    "    float thick = max(2.5, (t - prev) * 1.5);\n"
    "    if (raised(p, uv, q, thick)) {\n"
    "      float a = prev, b = t;\n"
    "      vec2 uvA = toUV(p + r * max(a, 0.02)), uvB = uv;\n"
    "      for (int k = 0; k < 6; k++) {\n"
    "        if (dot((uvB - uvA) / u_texel, (uvB - uvA) / u_texel) <= 1.0) break;\n"
    "        float m = 0.5 * (a + b);\n"
    "        vec3 qm = p + r * m;\n"
    "        vec2 uvm = toUV(qm);\n"
    "        if (raised(p, uvm, qm, thick)) { b = m; uvB = uvm; }\n"
    "        else { a = m; uvA = uvm; }\n"
    "      }\n"
    "      vec2 farC = cell(uvB), nearC = cell(uvA);\n"
    "      vec3 qB = p + r * b;\n"
    "      vec2 chosen = vec2(-1.0);\n"
    "      if (raised(p, farC, qB, thick)) chosen = farC;\n"
    "      else if (raised(p, nearC, qB, thick)) chosen = nearC;\n"
    "      else if (texture2D(u_depth, nearC).r <= 0.99999) chosen = nearC;\n"
    "      else if (texture2D(u_depth, farC).r <= 0.99999) chosen = farC;\n"
    "      if (chosen.x >= 0.0) {\n"
    "        hit = onWall(p, r, chosen);\n"
    "        t = b;\n"
    "        break;\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "  float fade = 1.0;\n"
    "  if (hit.x < 0.0) {\n"
    "    if (texture2D(u_depth, last).r <= 0.99999) return;\n"
    "    vec2 gap = skyAt.x >= 0.0 ? skyAt : last;\n"
    "    if (skyHole(p, gap)) return;\n"
    "    fade = smoothstep(0.05, 0.3, dot(r, u_up.xyz));\n"
    "    hit = last;\n"
    "  } else {\n"
    "    vec2 e = min(hit, 1.0 - hit);\n"
    "    fade = smoothstep(0.0, 0.08, min(e.x, e.y)) * (1.0 - smoothstep(150.0, 300.0, t));\n"
    "  }\n"
    "  float fresnel = 0.25 + 0.75 * pow(1.0 - max(0.0, dot(-v, n)), 4.0);\n"
    "  gl_FragColor = vec4(texture2D(u_tex, hit).rgb, level * fade * fresnel * u_strength);\n"
    "}\n";

const char* kTaa =
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_depth;\n"
    "uniform sampler2D u_history;\n"
    "uniform vec4 u_proj;\n"
    "uniform vec4 u_r0;\n"
    "uniform vec4 u_r1;\n"
    "uniform vec4 u_r2;\n"
    "uniform vec2 u_texel;\n"
    "uniform float u_reset;\n"
    "vec3 toYCoCg(vec3 c) {\n"
    "  return vec3(0.25 * c.r + 0.5 * c.g + 0.25 * c.b, 0.5 * c.r - 0.5 * c.b, -0.25 * c.r + 0.5 * c.g - 0.25 * c.b);\n"
    "}\n"
    "vec3 toRGB(vec3 c) { return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z); }\n"
    "vec3 history(vec2 uv) {\n"
    "  vec2 pos = uv / u_texel;\n"
    "  vec2 p1 = floor(pos - 0.5) + 0.5;\n"
    "  vec2 f = pos - p1;\n"
    "  vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));\n"
    "  vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);\n"
    "  vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));\n"
    "  vec2 w3 = f * f * (-0.5 + 0.5 * f);\n"
    "  vec2 w12 = w1 + w2;\n"
    "  vec2 t0 = (p1 - 1.0) * u_texel, t3 = (p1 + 2.0) * u_texel, t12 = (p1 + w2 / w12) * u_texel;\n"
    "  vec3 r = texture2D(u_history, vec2(t12.x, t0.y)).rgb * (w12.x * w0.y) +\n"
    "           texture2D(u_history, vec2(t0.x, t12.y)).rgb * (w0.x * w12.y) +\n"
    "           texture2D(u_history, t12).rgb * (w12.x * w12.y) +\n"
    "           texture2D(u_history, vec2(t3.x, t12.y)).rgb * (w3.x * w12.y) +\n"
    "           texture2D(u_history, vec2(t12.x, t3.y)).rgb * (w12.x * w3.y);\n"
    "  float w = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;\n"
    "  return max(r / w, 0.0);\n"
    "}\n"
    "void main() {\n"
    "  vec3 c = texture2D(u_tex, v_uv).rgb;\n"
    "  vec3 m1 = vec3(0.0), m2 = vec3(0.0);\n"
    "  for (int y = -1; y <= 1; y++)\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "      vec3 n = toYCoCg(texture2D(u_tex, v_uv + vec2(float(x), float(y)) * u_texel).rgb);\n"
    "      m1 += n;\n"
    "      m2 += n * n;\n"
    "    }\n"
    "  vec3 mean = m1 / 9.0;\n"
    "  vec3 sigma = sqrt(max(m2 / 9.0 - mean * mean, 0.0));\n"
    "  float d = texture2D(u_depth, v_uv).r * 2.0 - 1.0;\n"
    "  float z = 2.0 * u_proj.z * u_proj.w / ((u_proj.w - u_proj.z) * d - (u_proj.w + u_proj.z));\n"
    "  vec4 p = vec4((v_uv * 2.0 - 1.0) * u_proj.xy * -z, z, 1.0);\n"
    "  vec3 q = vec3(dot(u_r0, p), dot(u_r1, p), dot(u_r2, p));\n"
    "  vec2 prev = q.z < -1e-3 ? q.xy / (-q.z * u_proj.xy) * 0.5 + 0.5 : vec2(-1.0);\n"
    "  if (u_reset > 0.5 || prev.x < 0.0 || prev.x > 1.0 || prev.y < 0.0 || prev.y > 1.0) {\n"
    "    gl_FragColor = vec4(c, -z / u_proj.w);\n"
    "    return;\n"
    "  }\n"
    "  vec3 h = toRGB(clamp(toYCoCg(history(prev)), mean - 1.25 * sigma, mean + 1.25 * sigma));\n"
    "  float expected = -q.z / u_proj.w, stored = texture2D(u_history, prev).a;\n"
    "  float fresh = mix(0.1, 0.5, clamp(length((v_uv - prev) / u_texel) / 6.0, 0.0, 1.0));\n"
    "  if (abs(stored - expected) > 0.1 * expected + 0.01) fresh = 1.0;\n"
    "  gl_FragColor = vec4(mix(h, c, fresh), -z / u_proj.w);\n"
    "}\n";

const char* kMotion =
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_depth;\n"
    "uniform vec4 u_proj;\n"
    "uniform vec4 u_r0;\n"
    "uniform vec4 u_r1;\n"
    "uniform vec4 u_r2;\n"
    "uniform float u_amount;\n"
    "uniform vec2 u_aspect;\n"
    "void main() {\n"
    "  float d = texture2D(u_depth, v_uv).r * 2.0 - 1.0;\n"
    "  float z = 2.0 * u_proj.z * u_proj.w / ((u_proj.w - u_proj.z) * d - (u_proj.w + u_proj.z));\n"
    "  vec4 p = vec4((v_uv * 2.0 - 1.0) * u_proj.xy * -z, z, 1.0);\n"
    "  vec3 q = vec3(dot(u_r0, p), dot(u_r1, p), dot(u_r2, p));\n"
    "  vec2 prev = q.z < -1e-3 ? q.xy / (-q.z * u_proj.xy) * 0.5 + 0.5 : v_uv;\n"
    "  float edge = smoothstep(0.2, 0.6, length((v_uv - 0.5) * u_aspect));\n"
    "  vec2 v = (v_uv - prev) * u_amount * edge;\n"
    "  float len = length(v);\n"
    "  if (len > 0.04) v *= 0.04 / len;\n"
    "  vec3 c = vec3(0.0);\n"
    "  for (int i = 0; i < 8; i++) c += texture2D(u_tex, v_uv - v * (float(i) / 7.0 - 0.5)).rgb;\n"
    "  gl_FragColor = vec4(c / 8.0, 1.0);\n"
    "}\n";

const char* kComposite =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_scene;\n"
    "uniform sampler2D u_bloomNear;\n"
    "uniform sampler2D u_bloomFar;\n"
    "uniform sampler2D u_lut;\n"
    "uniform sampler2D u_aoTex;\n"
    "uniform sampler2D u_raysTex;\n"
    "uniform sampler2D u_ssrTex;\n"
    "uniform float u_strength;\n"
    "uniform float u_ao;\n"
    "uniform float u_rays;\n"
    "uniform float u_ssr;\n"
    "uniform vec2 u_aspect;\n"
    "uniform highp sampler2D u_depth;\n"
    "uniform vec4 u_flare;\n"
    "uniform vec4 u_gradeHigh;\n"
    "uniform vec4 u_gradeShadow;\n"
    "vec3 lut(vec3 c) {\n"
    "  float b = c.b * 15.0;\n"
    "  float s0 = floor(b);\n"
    "  float s1 = min(s0 + 1.0, 15.0);\n"
    "  vec2 uv = vec2((c.r * 15.0 + 0.5) / 256.0, (c.g * 15.0 + 0.5) / 16.0);\n"
    "  return mix(texture2D(u_lut, uv + vec2(s0 / 16.0, 0.0)).rgb,\n"
    "             texture2D(u_lut, uv + vec2(s1 / 16.0, 0.0)).rgb, b - s0);\n"
    "}\n"
    "vec3 ghost(vec2 pos, float r, vec3 col) {\n"
    "  return col * (1.0 - smoothstep(r * 0.5, r, length((v_uv - pos) * u_aspect)));\n"
    "}\n"
    "vec3 flare() {\n"
    "  float vis = 0.0;\n"
    "  for (int i = 0; i < 12; i++) {\n"
    "    float a = float(i) * 0.5235988;\n"
    "    highp float d = texture2D(u_depth, u_flare.xy + vec2(cos(a), sin(a)) * 0.012 / u_aspect).r;\n"
    "    vis += step(0.99999, d);\n"
    "  }\n"
    "  vec2 toCenter = vec2(0.5) - u_flare.xy;\n"
    "  vec3 f = vec3(1.0, 0.9, 0.75) * 0.35 * exp(-9.0 * length((v_uv - u_flare.xy) * u_aspect));\n"
    "  f += ghost(u_flare.xy + toCenter * 0.55, 0.035, vec3(0.25, 0.17, 0.08));\n"
    "  f += ghost(u_flare.xy + toCenter * 0.9, 0.07, vec3(0.10, 0.17, 0.25));\n"
    "  f += ghost(u_flare.xy + toCenter * 1.25, 0.025, vec3(0.2, 0.25, 0.15));\n"
    "  f += ghost(u_flare.xy + toCenter * 1.6, 0.12, vec3(0.07, 0.12, 0.2));\n"
    "  f += ghost(u_flare.xy + toCenter * 2.0, 0.05, vec3(0.22, 0.12, 0.1));\n"
    "  return f * (vis / 12.0) * u_flare.z;\n"
    "}\n"
    "void main() {\n"
    "  vec3 scene = texture2D(u_scene, v_uv).rgb * mix(1.0, texture2D(u_aoTex, v_uv).r, u_ao);\n"
    "  vec4 refl = texture2D(u_ssrTex, v_uv);\n"
    "  scene = mix(scene, refl.rgb, refl.a * u_ssr);\n"
    "  scene += texture2D(u_raysTex, v_uv).rgb * u_rays * (1.0 - scene);\n"
    "  vec3 bloom = texture2D(u_bloomNear, v_uv).rgb * 0.5 + texture2D(u_bloomFar, v_uv).rgb * 0.8;\n"
    "  vec3 c = (scene + bloom * u_strength * (1.0 - scene)) * (1.0 + 0.05 * u_strength);\n"
    "  float knee = 1.0 - 0.2 * u_strength;\n"
    "  vec3 over = max(c - knee, 0.0);\n"
    "  c = min(c, knee) + (1.0 - knee) * (1.0 - exp(-over / max(1.0 - knee, 0.001)));\n"
    "  c = mix(c, lut(clamp(c, 0.0, 1.0)), u_strength);\n"
    "  float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "  c = mix(vec3(lum), c, u_gradeHigh.a);\n"
    "  c = (c - 0.5) * u_gradeShadow.a + 0.5;\n"
    "  c = c * mix(vec3(1.0), u_gradeHigh.rgb, smoothstep(0.3, 1.0, lum)) +\n"
    "      u_gradeShadow.rgb * (1.0 - smoothstep(0.0, 0.5, lum));\n"
    "  if (u_flare.z > 0.0) c += flare();\n"
    "  vec2 d = (v_uv - 0.5) * u_aspect;\n"
    "  c *= 1.0 - 0.22 * u_strength * smoothstep(0.35, 0.85, length(d));\n"
    "  gl_FragColor = vec4(c, 1.0);\n"
    "}\n";

struct Target {
    GLuint tex = 0, fbo = 0;
    int w = 0, h = 0;
};

bool s_tried, s_ready, s_bloom, s_godRays, s_motion, s_flare, s_taa;
int s_grade;
float s_ssrStrength;
int s_aoMode;
GLuint s_bright, s_down, s_blur, s_composite, s_lut, s_aoProgram, s_aoBlur, s_aoUp, s_depthTex, s_white, s_raysMask,
    s_rays, s_motionProgram, s_ssrProgram, s_taaProgram;
Target s_scene, s_half, s_quarter[2], s_eighth[2], s_ao[2], s_aoFull, s_raysSrc, s_raysOut, s_moved, s_reflect, s_history[2];
const float kAoRadius = 1.5f;
const float kAoStrength = 0.6f;
const float kRaysStrength = 0.6f;
struct Grade {
    float high[4], shadow[4];
};
const Grade kGrades[] = {
    {{1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
    {{1.07f, 1.0f, 0.88f, 1.08f}, {0.03f, 0.01f, -0.02f, 1.03f}},
    {{0.92f, 0.98f, 1.08f, 0.92f}, {-0.02f, 0.0f, 0.04f, 1.03f}},
    {{1.02f, 1.0f, 0.97f, 0.9f}, {-0.02f, 0.01f, 0.05f, 1.18f}},
};
const float kWalkSpeed = 2.0f, kRunSpeed = 8.0f, kCameraCut = 3.0f;
float s_taaCamera[16];
bool s_taaHave;
int s_taaCur;
unsigned s_postFrames, s_jitterFrames;
float s_prevCamera[16], s_motionAmount;
bool s_havePrevCamera;
std::chrono::steady_clock::time_point s_prevTime;

bool makeTarget(Target& t, int w, int h) {
    t.w = std::max(1, w);
    t.h = std::max(1, h);
    gl.GenTextures(1, &t.tex);
    gl.BindTexture(GL_TEXTURE_2D, t.tex);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.GenFramebuffers(1, &t.fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
    return gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void defaultLut(std::vector<uint8_t>& px) {
    px.assign(256 * 16 * 4, 255);
    for (int g = 0; g < 16; g++)
        for (int b = 0; b < 16; b++)
            for (int r = 0; r < 16; r++) {
                float c[3] = {r / 15.0f, g / 15.0f, b / 15.0f};
                float l = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
                const float shadow[3] = {-0.012f, 0.0f, 0.03f};
                for (int i = 0; i < 3; i++) {
                    float v = l + (c[i] - l) * 1.15f;
                    v = v + 0.25f * (v * v * (3.0f - 2.0f * v) - v);
                    v += shadow[i] * (1.0f - l) * (1.0f - l);
                    c[i] = std::min(1.0f, std::max(0.0f, v));
                }
                uint8_t* p = &px[(size_t(g) * 256 + b * 16 + r) * 4];
                for (int i = 0; i < 3; i++) p[i] = uint8_t(lroundf(c[i] * 255.0f));
            }
}

void createLut() {
    std::vector<uint8_t> px;
    std::string path = files::dataDir() + "/lut.png";
    std::vector<unsigned char> bytes;
    gfx::Image image;
    if (files::read(path, &bytes) && gfx::decodePNG(bytes.data(), bytes.size(), image, false) && image.width == 256 &&
        image.height == 16) {
        px = image.rgba;
        fprintf(stderr, "medecomp: постобработка: LUT %s\n", path.c_str());
    } else {
        defaultLut(px);
    }
    gl.GenTextures(1, &s_lut);
    gl.BindTexture(GL_TEXTURE_2D, s_lut);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

GLuint whiteTexture() {
    static const uint8_t white[4] = {255, 255, 255, 255};
    GLuint t = 0;
    gl.GenTextures(1, &t);
    gl.BindTexture(GL_TEXTURE_2D, t);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return t;
}

bool createDepth() {
    gl.GenTextures(1, &s_depthTex);
    gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GLint(kDepth24), s_scene.w, s_scene.h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                  nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindFramebuffer(GL_FRAMEBUFFER, s_scene.fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_depthTex, 0);
    return gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool create() {
    s_tried = true;
    if (!glBlitFramebuffer_) {
        fprintf(stderr, "medecomp: постобработка — нужен OpenGL ES 3.0, выключена\n");
        return false;
    }
    s_composite = link(kVertex, kComposite);
    int w = framebufferPixelWidth(), h = framebufferPixelHeight();
    bool ok = s_composite && makeTarget(s_scene, w, h) && (!(s_aoMode || s_godRays || s_motion || s_ssrStrength > 0 || s_flare || s_taa) || createDepth());
    if (ok && s_bloom) {
        s_bright = link(kVertex, kBright);
        s_down = link(kVertex, kDown);
        s_blur = link(kVertex, kBlur);
        ok = s_bright && s_down && s_blur && makeTarget(s_half, w / 2, h / 2) &&
             makeTarget(s_quarter[0], w / 4, h / 4) && makeTarget(s_quarter[1], w / 4, h / 4) &&
             makeTarget(s_eighth[0], w / 8, h / 8) && makeTarget(s_eighth[1], w / 8, h / 8);
    }
    if (ok && s_aoMode) {
        bool half = s_aoMode > 1;
        std::string fs = std::string(s_aoMode == 3 ? "#define GTAO\n" : "") + kAoCommon + (half ? kHorizon : kAo);
        s_aoProgram = link(kVertex, fs.c_str());
        s_aoBlur = link(kVertex, kAoBlur);
        int aw = half ? (w + 1) / 2 : w, ah = half ? (h + 1) / 2 : h;
        ok = s_aoProgram && s_aoBlur && makeTarget(s_ao[0], aw, ah) && makeTarget(s_ao[1], aw, ah);
        if (ok && half) {
            s_aoUp = link(kVertex, kAoUp);
            ok = s_aoUp && makeTarget(s_aoFull, w, h);
        }
    }
    if (ok && s_godRays) {
        s_raysMask = link(kVertex, kRaysMask);
        s_rays = link(kVertex, kRays);
        ok = s_raysMask && s_rays && makeTarget(s_raysSrc, w / 4, h / 4) && makeTarget(s_raysOut, w / 4, h / 4);
    }
    if (ok && s_ssrStrength > 0) {
        s_ssrProgram = link(kVertex, (std::string(kAoCommon) + kSsr).c_str());
        ok = s_ssrProgram && makeTarget(s_reflect, w, h);
    }
    if (ok && s_taa) {
        s_taaProgram = link(kVertex, kTaa);
        ok = s_taaProgram && makeTarget(s_history[0], w, h) && makeTarget(s_history[1], w, h);
    }
    if (ok && s_motion) {
        s_motionProgram = link(kVertex, kMotion);
        ok = s_motionProgram && makeTarget(s_moved, w, h);
    }
    if (!ok) {
        fprintf(stderr, "medecomp: постобработка не собрана — выключена\n");
        return false;
    }
    s_white = whiteTexture();
    createLut();
    if (s_bloom) fprintf(stderr, "medecomp: постобработка: улучшенная, сила %.2f\n", postStrength());
    const char* aoNames[] = {"", "SSAO", "HBAO, половинное разрешение", "GTAO, половинное разрешение"};
    if (s_aoMode) fprintf(stderr, "medecomp: затенение углов %s\n", aoNames[s_aoMode]);
    if (s_godRays) fprintf(stderr, "medecomp: лучи солнца\n");
    if (s_motion) fprintf(stderr, "medecomp: размытие в движении\n");
    if (s_ssrStrength > 0) fprintf(stderr, "medecomp: отражения на полах, сила %.2f\n", s_ssrStrength);
    if (s_flare) fprintf(stderr, "medecomp: блик солнца\n");
    if (s_taa) fprintf(stderr, "medecomp: сглаживание TAA\n");
    const char* gradeNames[] = {"", "тёплый", "холодный", "как в 2008"};
    if (s_grade) fprintf(stderr, "medecomp: цветокоррекция: %s\n", gradeNames[s_grade]);
    return true;
}

void pass(GLuint p, const Target& src, const Target& dst) {
    gl.BindFramebuffer(GL_FRAMEBUFFER, dst.fbo);
    gl.Viewport(0, 0, dst.w, dst.h);
    gl.UseProgram(p);
    gl.BindTexture(GL_TEXTURE_2D, src.tex);
    gl.Uniform1i(gl.GetUniformLocation(p, "u_tex"), 0);
    gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void blur(Target* t) {
    GLint dir = gl.GetUniformLocation(s_blur, "u_dir");
    gl.UseProgram(s_blur);
    gl.Uniform2f(dir, 1.0f / t[0].w, 0.0f);
    pass(s_blur, t[0], t[1]);
    gl.UseProgram(s_blur);
    gl.Uniform2f(dir, 0.0f, 1.0f / t[1].h);
    pass(s_blur, t[1], t[0]);
}

void down(GLuint p, const Target& src, const Target& dst) {
    gl.UseProgram(p);
    gl.Uniform2f(gl.GetUniformLocation(p, "u_texel"), 1.0f / src.w, 1.0f / src.h);
    pass(p, src, dst);
}

}

int postSetting() { return settings::get("post", "ipad") == "enhanced" ? 1 : 0; }

namespace {
const char* const kAoKeys[] = {"off", "on", "hbao", "gtao"};
}

int ssaoSetting() {
    std::string v = settings::get("ssao", "off");
    for (int i = 1; i < 4; i++)
        if (v == kAoKeys[i]) return i;
    return 0;
}

void setSsaoSetting(int mode) { settings::set("ssao", kAoKeys[mode >= 0 && mode < 4 ? mode : 0]); }

bool godRaysSetting() { return settings::get("god_rays", "off") == "on"; }

void setGodRaysSetting(bool on) { settings::set("god_rays", on ? "on" : "off"); }

bool motionBlurSetting() { return settings::get("motion_blur", "off") == "on"; }

void setMotionBlurSetting(bool on) { settings::set("motion_blur", on ? "on" : "off"); }

float ssrStrengthSetting() {
    float v = float(atof(settings::get("ssr_strength", "0.25").c_str()));
    return v < 0.05f ? 0.05f : v > 1 ? 1 : v;
}

float ssrSetting() { return settings::get("ssr", "off") == "on" ? ssrStrengthSetting() : 0.0f; }

void setSsrSetting(bool on, float strength) {
    settings::set("ssr", on ? "on" : "off");
    char buf[16];
    snprintf(buf, sizeof buf, "%.2f", strength);
    settings::set("ssr_strength", buf);
}

bool lensFlareSetting() { return settings::get("lens_flare", "off") == "on"; }

void setLensFlareSetting(bool on) { settings::set("lens_flare", on ? "on" : "off"); }

namespace {
const char* const kGradeKeys[] = {"off", "warm", "cold", "2008"};
}

int gradeSetting() {
    std::string v = settings::get("grade", "off");
    for (int i = 1; i < 4; i++)
        if (v == kGradeKeys[i]) return i;
    return 0;
}

void setGradeSetting(int preset) { settings::set("grade", kGradeKeys[preset >= 0 && preset < 4 ? preset : 0]); }

bool taaSetting() { return settings::get("taa", "off") == "on"; }

void setTaaSetting(bool on) { settings::set("taa", on ? "on" : "off"); }

bool postNeedsDepth() {
    return !originalGraphics() && (ssaoSetting() || godRaysSetting() || motionBlurSetting() || ssrSetting() > 0 ||
                                   lensFlareSetting() || taaSetting());
}

float postStrength() {
    float v = float(atof(settings::get("post_strength", "0.5").c_str()));
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

void setPostSetting(int mode, float strength) {
    settings::set("post", mode ? "enhanced" : "ipad");
    char buf[16];
    snprintf(buf, sizeof buf, "%.2f", strength);
    settings::set("post_strength", buf);
}

}
}

bool medecomp_taa_jitter(float out[2]) {
    using namespace port::platform;
    if (!s_ready || !s_taa || s_postFrames == s_jitterFrames) return false;
    s_jitterFrames = s_postFrames;
    auto halton = [](unsigned i, unsigned base) {
        float f = 1.0f, r = 0.0f;
        for (; i; i /= base) {
            f /= float(base);
            r += f * float(i % base);
        }
        return r;
    };
    unsigned i = s_postFrames % 8 + 1;
    out[0] = (halton(i, 2) - 0.5f) * 2.0f / float(framebufferPixelWidth());
    out[1] = (halton(i, 3) - 0.5f) * 2.0f / float(framebufferPixelHeight());
    return true;
}

extern "C" void medecomp_post_world(float fovy, float aspect, float nearPlane, float farPlane) {
    using namespace port::platform;
    if (!s_tried) {
        bool original = originalGraphics();
        s_bloom = !original && postSetting() != 0;
        s_aoMode = original ? 0 : ssaoSetting();
        s_godRays = !original && godRaysSetting();
        s_motion = !original && motionBlurSetting();
        s_ssrStrength = original ? 0.0f : ssrSetting();
        s_flare = !original && lensFlareSetting();
        s_grade = original ? 0 : gradeSetting();
        s_taa = !original && taaSetting();
        if (!s_bloom && !s_aoMode && !s_godRays && !s_motion && s_ssrStrength <= 0 && !s_flare && !s_grade && !s_taa) {
            s_tried = true;
            return;
        }
        GLint fbo, tex;
        gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
        gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
        s_ready = create();
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.BindTexture(GL_TEXTURE_2D, tex);
    }
    if (!s_ready) return;

    GLint draw, read, program, active, tex[8], array, viewport[4];
    GLboolean mask[4], depthMask;
    gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &draw);
    gl.GetIntegerv(0x8caa, &read);
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &program);
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &active);
    for (int i = 0; i < 8; i++) {
        gl.ActiveTexture(GL_TEXTURE0 + i);
        gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex[i]);
    }
    gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    gl.GetIntegerv(GL_VIEWPORT, viewport);
    gl.GetBooleanv(GL_COLOR_WRITEMASK, mask);
    gl.GetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const GLenum caps[] = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_DITHER};
    GLboolean was[6];
    for (int i = 0; i < 6; i++) {
        was[i] = gl.IsEnabled(caps[i]);
        gl.Disable(caps[i]);
    }
    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl.DepthMask(GL_FALSE);

    gl.BindFramebuffer(kReadFramebuffer, draw);
    gl.BindFramebuffer(kDrawFramebuffer, s_scene.fbo);
    glBlitFramebuffer_(0, 0, s_scene.w, s_scene.h, 0, 0, s_scene.w, s_scene.h,
                       GL_COLOR_BUFFER_BIT | (s_depthTex ? GL_DEPTH_BUFFER_BIT : 0), GL_NEAREST);

    static const GLfloat pos[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    static const GLfloat uv[] = {0, 0, 1, 0, 0, 1, 1, 1};
    port::gfx::releaseGameArrays();
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.EnableVertexAttribArray(s_attrPos);
    gl.EnableVertexAttribArray(s_attrUV);
    gl.VertexAttribPointer(s_attrPos, 2, GL_FLOAT, GL_FALSE, 0, pos);
    gl.VertexAttribPointer(s_attrUV, 2, GL_FLOAT, GL_FALSE, 0, uv);
    gl.ActiveTexture(GL_TEXTURE0);

    s_postFrames++;
    if (s_taa) {
        float cur[16], speed = 0.0f;
        bool inRun = gameCamera(cur, &speed);
        float jump = inRun && s_taaHave ? hypotf(hypotf(cur[3] - s_taaCamera[3], cur[7] - s_taaCamera[7]),
                                                 cur[11] - s_taaCamera[11])
                                        : 1e9f;
        bool reset = !inRun || jump > kCameraCut;
        float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        if (!reset) {
            m3g::Transform toPrev, now3;
            toPrev.set(s_taaCamera);
            now3.set(cur);
            toPrev.invert();
            toPrev.postMultiply(&now3);
            toPrev.get(m);
        }
        float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
        const Target& out = s_history[s_taaCur];
        gl.ActiveTexture(GL_TEXTURE1);
        gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
        gl.ActiveTexture(GL_TEXTURE2);
        gl.BindTexture(GL_TEXTURE_2D, s_history[1 - s_taaCur].tex);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.UseProgram(s_taaProgram);
        gl.Uniform1i(gl.GetUniformLocation(s_taaProgram, "u_depth"), 1);
        gl.Uniform1i(gl.GetUniformLocation(s_taaProgram, "u_history"), 2);
        gl.Uniform4f(gl.GetUniformLocation(s_taaProgram, "u_proj"), t * aspect, t, nearPlane, farPlane);
        gl.Uniform4f(gl.GetUniformLocation(s_taaProgram, "u_r0"), m[0], m[1], m[2], m[3]);
        gl.Uniform4f(gl.GetUniformLocation(s_taaProgram, "u_r1"), m[4], m[5], m[6], m[7]);
        gl.Uniform4f(gl.GetUniformLocation(s_taaProgram, "u_r2"), m[8], m[9], m[10], m[11]);
        gl.Uniform2f(gl.GetUniformLocation(s_taaProgram, "u_texel"), 1.0f / s_scene.w, 1.0f / s_scene.h);
        gl.Uniform1f(gl.GetUniformLocation(s_taaProgram, "u_reset"), reset ? 1.0f : 0.0f);
        pass(s_taaProgram, s_scene, out);
        gl.BindFramebuffer(kReadFramebuffer, out.fbo);
        gl.BindFramebuffer(kDrawFramebuffer, s_scene.fbo);
        glBlitFramebuffer_(0, 0, s_scene.w, s_scene.h, 0, 0, s_scene.w, s_scene.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        s_taaCur = 1 - s_taaCur;
        s_taaHave = inRun;
        if (inRun) std::copy(cur, cur + 16, s_taaCamera);
    }

    if (s_bloom) {
        gl.UseProgram(s_bright);
        gl.Uniform1f(gl.GetUniformLocation(s_bright, "u_threshold"), 0.8f);
        down(s_bright, s_scene, s_half);
        down(s_down, s_half, s_quarter[0]);
        blur(s_quarter);
        down(s_down, s_quarter[0], s_eighth[0]);
        blur(s_eighth);
    }
    if (s_aoMode) {
        float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
        gl.ActiveTexture(GL_TEXTURE1);
        gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
        gl.ActiveTexture(GL_TEXTURE0);
        for (GLuint p : {s_aoProgram, s_aoBlur, s_aoUp}) {
            if (!p) continue;
            gl.UseProgram(p);
            gl.Uniform1i(gl.GetUniformLocation(p, "u_depth"), 1);
            gl.Uniform2f(gl.GetUniformLocation(p, "u_texel"), 1.0f / s_scene.w, 1.0f / s_scene.h);
            gl.Uniform4f(gl.GetUniformLocation(p, "u_proj"), t * aspect, t, nearPlane, farPlane);
            gl.Uniform2f(gl.GetUniformLocation(p, "u_aoTexel"), 1.0f / s_ao[0].w, 1.0f / s_ao[0].h);
        }
        gl.UseProgram(s_aoProgram);
        gl.Uniform1f(gl.GetUniformLocation(s_aoProgram, "u_radius"), kAoRadius);
        pass(s_aoProgram, s_scene, s_ao[0]);
        gl.UseProgram(s_aoBlur);
        gl.Uniform1f(gl.GetUniformLocation(s_aoBlur, "u_scale"), s_aoUp ? 2.0f : 1.0f);
        GLint dir = gl.GetUniformLocation(s_aoBlur, "u_dir");
        gl.Uniform2f(dir, 1.0f, 0.0f);
        pass(s_aoBlur, s_ao[0], s_ao[1]);
        gl.UseProgram(s_aoBlur);
        gl.Uniform2f(dir, 0.0f, 1.0f);
        pass(s_aoBlur, s_ao[1], s_ao[0]);
        if (s_aoUp) pass(s_aoUp, s_ao[0], s_aoFull);
    }

    float camera[16], speed;
    bool reflect = s_ssrStrength > 0 && gameCamera(camera, &speed);
    if (reflect) {
        float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
        gl.ActiveTexture(GL_TEXTURE1);
        gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.UseProgram(s_ssrProgram);
        gl.Uniform1i(gl.GetUniformLocation(s_ssrProgram, "u_depth"), 1);
        gl.Uniform2f(gl.GetUniformLocation(s_ssrProgram, "u_texel"), 1.0f / s_scene.w, 1.0f / s_scene.h);
        gl.Uniform4f(gl.GetUniformLocation(s_ssrProgram, "u_proj"), t * aspect, t, nearPlane, farPlane);
        gl.Uniform4f(gl.GetUniformLocation(s_ssrProgram, "u_up"), camera[4], camera[5], camera[6], 0.0f);
        gl.Uniform1f(gl.GetUniformLocation(s_ssrProgram, "u_strength"), s_ssrStrength);
        pass(s_ssrProgram, s_scene, s_reflect);
    }

    float big = float(std::max(s_scene.w, s_scene.h));
    float rays = 0.0f, flare = 0.0f;
    float sun[3], su = 0.0f, sv = 0.0f;
    if ((s_godRays || s_flare) && sunInCamera(sun) && sun[2] < -0.05f) {
        float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
        su = 0.5f + 0.5f * sun[0] / (-sun[2] * t * aspect), sv = 0.5f + 0.5f * sun[1] / (-sun[2] * t);
        float out = std::max(std::max(-su, su - 1.0f), std::max(-sv, sv - 1.0f));
        if (s_flare) flare = 1.0f - std::min(1.0f, std::max(0.0f, out + 0.03f) / 0.03f);
        if (s_godRays) rays = kRaysStrength * (1.0f - std::min(1.0f, std::max(0.0f, out) / 0.4f));
        if (rays > 0.0f) {
            gl.ActiveTexture(GL_TEXTURE1);
            gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.UseProgram(s_raysMask);
            gl.Uniform1i(gl.GetUniformLocation(s_raysMask, "u_depth"), 1);
            gl.Uniform2f(gl.GetUniformLocation(s_raysMask, "u_sun"), su, sv);
            gl.Uniform2f(gl.GetUniformLocation(s_raysMask, "u_aspect"), s_scene.w / big, s_scene.h / big);
            pass(s_raysMask, s_scene, s_raysSrc);
            gl.UseProgram(s_rays);
            gl.Uniform2f(gl.GetUniformLocation(s_rays, "u_sun"), su, sv);
            pass(s_rays, s_raysSrc, s_raysOut);
        }
    }

    bool moved = false;
    if (s_motion) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - s_prevTime).count();
        s_prevTime = now;
        float cur[16], speed = 0.0f;
        bool inRun = gameCamera(cur, &speed);
        float target = inRun ? std::min(1.0f, std::max(0.0f, (speed - kWalkSpeed) / (kRunSpeed - kWalkSpeed))) : 0.0f;
        s_motionAmount += (target - s_motionAmount) * std::min(1.0f, dt * 6.0f);
        float jump = inRun && s_havePrevCamera ? hypotf(hypotf(cur[3] - s_prevCamera[3], cur[7] - s_prevCamera[7]),
                                                        cur[11] - s_prevCamera[11])
                                               : 1e9f;
        if (inRun && s_havePrevCamera && jump < kCameraCut && dt > 0.0f && s_motionAmount > 0.01f) {
            m3g::Transform toPrev, now3;
            toPrev.set(s_prevCamera);
            now3.set(cur);
            toPrev.invert();
            toPrev.postMultiply(&now3);
            float m[16];
            toPrev.get(m);
            float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
            gl.ActiveTexture(GL_TEXTURE1);
            gl.BindTexture(GL_TEXTURE_2D, s_depthTex);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.UseProgram(s_motionProgram);
            gl.Uniform1i(gl.GetUniformLocation(s_motionProgram, "u_depth"), 1);
            gl.Uniform4f(gl.GetUniformLocation(s_motionProgram, "u_proj"), t * aspect, t, nearPlane, farPlane);
            gl.Uniform4f(gl.GetUniformLocation(s_motionProgram, "u_r0"), m[0], m[1], m[2], m[3]);
            gl.Uniform4f(gl.GetUniformLocation(s_motionProgram, "u_r1"), m[4], m[5], m[6], m[7]);
            gl.Uniform4f(gl.GetUniformLocation(s_motionProgram, "u_r2"), m[8], m[9], m[10], m[11]);
            float shutter = std::min(4.0f, std::max(0.25f, (1.0f / 60.0f) / dt));
            gl.Uniform1f(gl.GetUniformLocation(s_motionProgram, "u_amount"), s_motionAmount * shutter);
            gl.Uniform2f(gl.GetUniformLocation(s_motionProgram, "u_aspect"), s_scene.w / big, s_scene.h / big);
            pass(s_motionProgram, s_scene, s_moved);
            moved = true;
        }
        s_havePrevCamera = inRun;
        if (inRun) std::copy(cur, cur + 16, s_prevCamera);
    }

    gl.BindFramebuffer(GL_FRAMEBUFFER, draw);
    gl.Viewport(0, 0, s_scene.w, s_scene.h);
    gl.UseProgram(s_composite);
    const GLuint units[8] = {moved ? s_moved.tex : s_scene.tex,
                             s_bloom ? s_quarter[0].tex : s_white,
                             s_bloom ? s_eighth[0].tex : s_white,
                             s_lut,
                             !s_aoMode ? s_white : s_aoUp ? s_aoFull.tex : s_ao[0].tex,
                             rays > 0.0f ? s_raysOut.tex : s_white,
                             reflect ? s_reflect.tex : s_white,
                             s_depthTex ? s_depthTex : s_white};
    const char* names[8] = {"u_scene", "u_bloomNear", "u_bloomFar", "u_lut", "u_aoTex", "u_raysTex", "u_ssrTex", "u_depth"};
    for (int i = 7; i >= 0; i--) {
        gl.ActiveTexture(GL_TEXTURE0 + i);
        gl.BindTexture(GL_TEXTURE_2D, units[i]);
        gl.Uniform1i(gl.GetUniformLocation(s_composite, names[i]), i);
    }
    gl.Uniform1f(gl.GetUniformLocation(s_composite, "u_strength"), s_bloom ? postStrength() : 0.0f);
    gl.Uniform1f(gl.GetUniformLocation(s_composite, "u_ao"), s_aoMode ? kAoStrength : 0.0f);
    gl.Uniform1f(gl.GetUniformLocation(s_composite, "u_rays"), rays);
    gl.Uniform1f(gl.GetUniformLocation(s_composite, "u_ssr"), reflect ? 1.0f : 0.0f);
    gl.Uniform4f(gl.GetUniformLocation(s_composite, "u_flare"), su, sv, flare, 0.0f);
    const Grade& grade = kGrades[s_grade];
    gl.Uniform4f(gl.GetUniformLocation(s_composite, "u_gradeHigh"), grade.high[0], grade.high[1], grade.high[2], grade.high[3]);
    gl.Uniform4f(gl.GetUniformLocation(s_composite, "u_gradeShadow"), grade.shadow[0], grade.shadow[1], grade.shadow[2],
                 grade.shadow[3]);
    gl.Uniform2f(gl.GetUniformLocation(s_composite, "u_aspect"), s_scene.w / big, s_scene.h / big);
    gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (int i = 7; i >= 0; i--) {
        gl.ActiveTexture(GL_TEXTURE0 + i);
        gl.BindTexture(GL_TEXTURE_2D, tex[i]);
    }
    gl.ActiveTexture(active);
    gl.DisableVertexAttribArray(s_attrPos);
    gl.DisableVertexAttribArray(s_attrUV);
    gl.BindBuffer(GL_ARRAY_BUFFER, array);
    gl.BindFramebuffer(kDrawFramebuffer, draw);
    gl.BindFramebuffer(kReadFramebuffer, read);
    gl.Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    gl.ColorMask(mask[0], mask[1], mask[2], mask[3]);
    gl.DepthMask(depthMask);
    gl.UseProgram(program);
    for (int i = 0; i < 6; i++)
        if (was[i]) gl.Enable(caps[i]);
}
