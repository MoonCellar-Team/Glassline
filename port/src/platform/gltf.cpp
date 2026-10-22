#include "gltf.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include "../ea/json.h"

using port::online::Json;

namespace port {
namespace platform {
namespace gltf {

namespace {

bool s_russian = false;

}

void setRussian(bool russian) { s_russian = russian; }
std::string T(const char* ru, const char* en) { return s_russian ? ru : en; }

namespace {

const uint32_t kMagic = 0x46546C67;
const uint32_t kChunkJson = 0x4E4F534A, kChunkBin = 0x004E4942;

bool readFile(const std::string& path, std::string* out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    std::ostringstream s;
    s << in.rdbuf();
    *out = s.str();
    return true;
}

uint32_t u32(const std::string& s, size_t at) {
    uint32_t v;
    memcpy(&v, s.data() + at, 4);
    return v;
}

int components(const std::string& type) {
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    if (type == "VEC4") return 4;
    if (type == "MAT4") return 16;
    return 0;
}

int componentSize(int type) {
    switch (type) {
    case 5120: case 5121: return 1;
    case 5122: case 5123: return 2;
    case 5125: case 5126: return 4;
    default: return 0;
    }
}

struct Reader {
    Json doc;
    std::vector<std::string> buffers;
    std::string dir;
    std::string error;

    bool fail(const std::string& why) {
        error = why;
        return false;
    }

    bool view(int index, const char** data, size_t* length, size_t* stride) {
        const Json& v = doc["bufferViews"][size_t(index)];
        if (v.type != Json::Object) return fail(T("нет bufferView ", "no bufferView ") + std::to_string(index));
        size_t b = size_t(v["buffer"].integer());
        if (b >= buffers.size()) return fail(T("bufferView ссылается на несуществующий буфер", "bufferView refers to a missing buffer"));
        size_t offset = size_t(v["byteOffset"].integer()), len = size_t(v["byteLength"].integer());
        if (offset + len > buffers[b].size()) return fail(T("bufferView за концом буфера", "bufferView past the end of the buffer"));
        *data = buffers[b].data() + offset;
        *length = len;
        *stride = size_t(v["byteStride"].integer());
        return true;
    }

    bool accessor(int index, std::vector<double>* out, int* comps) {
        const Json& a = doc["accessors"][size_t(index)];
        if (a.type != Json::Object) return fail(T("нет accessor ", "no accessor ") + std::to_string(index));
        int n = components(a["type"].str());
        int type = int(a["componentType"].integer());
        int size = componentSize(type);
        size_t count = size_t(a["count"].integer());
        if (!n || !size) return fail("accessor " + std::to_string(index) + T(": тип не понять", ": unknown type"));
        *comps = n;
        out->assign(count * size_t(n), 0.0);
        if (a["bufferView"].type == Json::Null) return true;
        const char* data;
        size_t length, stride;
        if (!view(int(a["bufferView"].integer()), &data, &length, &stride)) return false;
        size_t offset = size_t(a["byteOffset"].integer());
        size_t elem = size_t(n) * size_t(size);
        if (!stride) stride = elem;
        if (count && offset + (count - 1) * stride + elem > length) return fail(T("accessor за концом данных", "accessor past the end of the data"));
        bool norm = a["normalized"].boolean;
        for (size_t i = 0; i < count; i++)
            for (int c = 0; c < n; c++) {
                const char* p = data + offset + i * stride + size_t(c) * size_t(size);
                double v = 0;
                switch (type) {
                case 5120: v = *reinterpret_cast<const int8_t*>(p); if (norm) v = std::max(v / 127.0, -1.0); break;
                case 5121: v = *reinterpret_cast<const uint8_t*>(p); if (norm) v /= 255.0; break;
                case 5122: { int16_t x; memcpy(&x, p, 2); v = x; if (norm) v = std::max(v / 32767.0, -1.0); break; }
                case 5123: { uint16_t x; memcpy(&x, p, 2); v = x; if (norm) v /= 65535.0; break; }
                case 5125: { uint32_t x; memcpy(&x, p, 4); v = x; break; }
                case 5126: { float x; memcpy(&x, p, 4); v = x; break; }
                }
                (*out)[i * size_t(n) + size_t(c)] = v;
            }
        return true;
    }

    bool loadUri(const std::string& uri, std::string* out) {
        const std::string marker = ";base64,";
        if (uri.compare(0, 5, "data:") == 0) {
            size_t at = uri.find(marker);
            return at != std::string::npos && port::online::base64Decode(uri.substr(at + marker.size()), out);
        }
        std::string name;
        for (size_t i = 0; i < uri.size(); i++) {
            if (uri[i] == '%' && i + 2 < uri.size()) {
                name += char(strtol(uri.substr(i + 1, 2).c_str(), nullptr, 16));
                i += 2;
            } else {
                name += uri[i];
            }
        }
        return readFile(dir + name, out);
    }
};

void nodeMatrix(const Json& node, float* m) {
    const Json& mat = node["matrix"];
    if (mat.type == Json::Array && mat.items.size() == 16) {
        for (int i = 0; i < 16; i++) m[i] = float(mat[size_t(i)].number);
        return;
    }
    double t[3] = {0, 0, 0}, s[3] = {1, 1, 1}, q[4] = {0, 0, 0, 1};
    for (int i = 0; i < 3; i++) {
        if (node["translation"].items.size() == 3) t[i] = node["translation"][size_t(i)].number;
        if (node["scale"].items.size() == 3) s[i] = node["scale"][size_t(i)].number;
    }
    if (node["rotation"].items.size() == 4)
        for (int i = 0; i < 4; i++) q[i] = node["rotation"][size_t(i)].number;
    double x = q[0], y = q[1], z = q[2], w = q[3];
    double r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w),
                   2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w),
                   2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)};
    for (int c = 0; c < 3; c++)
        for (int row = 0; row < 3; row++) m[c * 4 + row] = float(r[c * 3 + row] * s[c]);
    m[3] = m[7] = m[11] = 0;
    m[12] = float(t[0]), m[13] = float(t[1]), m[14] = float(t[2]), m[15] = 1;
}

void multiply(const float* a, const float* b, float* out) {
    float m[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float v = 0;
            for (int k = 0; k < 4; k++) v += a[k * 4 + r] * b[c * 4 + k];
            m[c * 4 + r] = v;
        }
    memcpy(out, m, sizeof m);
}

void loadImage(Reader& r, const Json& image, Image* out) {
    out->mime = image["mimeType"].str();
    if (image["bufferView"].type != Json::Null) {
        const char* data;
        size_t length, stride;
        if (r.view(int(image["bufferView"].integer()), &data, &length, &stride)) out->bytes.assign(data, length);
    } else if (image["uri"].type != Json::Null) {
        r.loadUri(image["uri"].str(), &out->bytes);
    }
}

}

bool readScene(const std::string& path, Scene* out, std::string* error) {
    Reader r;
    size_t slash = path.find_last_of("/\\");
    r.dir = slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
    std::string file;
    if (!readFile(path, &file)) return *error = T("не открыть ", "cannot open ") + path, false;
    std::string json;
    if (file.size() >= 12 && u32(file, 0) == kMagic) {
        size_t at = 12;
        while (at + 8 <= file.size()) {
            uint32_t len = u32(file, at), type = u32(file, at + 4);
            if (at + 8 + len > file.size()) break;
            if (type == kChunkJson) json = file.substr(at + 8, len);
            else if (type == kChunkBin && r.buffers.empty()) r.buffers.push_back(file.substr(at + 8, len));
            at += 8 + ((len + 3) & ~3u);
        }
    } else {
        json = file;
    }
    if (!port::online::parseJson(json, &r.doc) || r.doc.type != Json::Object)
        return *error = T("не glTF: JSON не разобрать", "not glTF: the JSON cannot be parsed"), false;
    if (r.doc["asset"]["version"].str().compare(0, 1, "2") != 0) return *error = T("нужен glTF 2.0", "glTF 2.0 is required"), false;
    for (const Json& ext : r.doc["extensionsRequired"].items)
        if (ext.str() == "KHR_draco_mesh_compression" || ext.str() == "EXT_meshopt_compression")
            return *error = T("сетка сжата (", "the mesh is compressed (") + ext.str() +
                            T("): выгрузите без сжатия", "): export it without compression"), false;
    const Json& buffers = r.doc["buffers"];
    for (size_t i = 0; i < buffers.items.size(); i++) {
        if (i == 0 && !r.buffers.empty() && buffers[i]["uri"].type == Json::Null) continue;
        std::string data;
        if (!r.loadUri(buffers[i]["uri"].str(), &data)) return *error = T("не прочитать буфер ", "cannot read buffer ") + buffers[i]["uri"].str(), false;
        if (i < r.buffers.size()) r.buffers[i] = data;
        else r.buffers.push_back(data);
    }

    const Json& nodes = r.doc["nodes"];
    out->nodes.assign(nodes.items.size(), Node());
    for (size_t i = 0; i < nodes.items.size(); i++) {
        Node& n = out->nodes[i];
        n.name = nodes[i]["name"].str();
        nodeMatrix(nodes[i], n.local);
        if (nodes[i]["mesh"].type != Json::Null) n.mesh = int(nodes[i]["mesh"].integer());
        if (nodes[i]["skin"].type != Json::Null) n.skin = int(nodes[i]["skin"].integer());
        for (const Json& child : nodes[i]["children"].items) {
            size_t c = size_t(child.integer());
            if (c < nodes.items.size() && c != i) out->nodes[c].parent = int(i);
        }
    }
    std::vector<int> state(out->nodes.size(), 0);
    for (size_t i = 0; i < out->nodes.size(); i++) {
        std::vector<int> chain;
        for (int k = int(i); k >= 0 && state[size_t(k)] == 0; k = out->nodes[size_t(k)].parent) {
            state[size_t(k)] = 1;
            chain.push_back(k);
        }
        for (size_t c = chain.size(); c-- > 0;) {
            Node& n = out->nodes[size_t(chain[c])];
            if (n.parent >= 0 && state[size_t(n.parent)] != 2) n.parent = -1;
            if (n.parent >= 0) multiply(out->nodes[size_t(n.parent)].world, n.local, n.world);
            else memcpy(n.world, n.local, sizeof n.world);
            state[size_t(chain[c])] = 2;
        }
    }

    for (const Json& s : r.doc["skins"].items) {
        Skin skin;
        for (const Json& j : s["joints"].items)
            if (j.integer() >= 0 && size_t(j.integer()) < out->nodes.size()) skin.joints.push_back(int(j.integer()));
        skin.inverseBind.assign(skin.joints.size() * 16, 0.0f);
        for (size_t b = 0; b < skin.joints.size(); b++)
            for (int i = 0; i < 4; i++) skin.inverseBind[b * 16 + size_t(i) * 5] = 1.0f;
        if (s["inverseBindMatrices"].type != Json::Null) {
            std::vector<double> m;
            int n;
            if (!r.accessor(int(s["inverseBindMatrices"].integer()), &m, &n) || n != 16 || m.size() < skin.inverseBind.size())
                return *error = r.error.empty() ? T("inverseBindMatrices не те", "bad inverseBindMatrices") : r.error, false;
            for (size_t i = 0; i < skin.inverseBind.size(); i++) skin.inverseBind[i] = float(m[i]);
        }
        out->skins.push_back(skin);
    }

    for (const Json& m : r.doc["meshes"].items) {
        Mesh mesh;
        mesh.name = m["name"].str();
        for (const Json& prim : m["primitives"].items) {
            if (prim["mode"].type != Json::Null && prim["mode"].integer() != 4) continue;
            const Json& attrs = prim["attributes"];
            if (attrs["POSITION"].type == Json::Null) continue;
            Primitive p;
            std::vector<double> pos, uv, idx;
            int n;
            if (!r.accessor(int(attrs["POSITION"].integer()), &pos, &n) || n != 3) return *error = r.error, false;
            size_t count = pos.size() / 3;
            if (attrs["TEXCOORD_0"].type != Json::Null) {
                if (!r.accessor(int(attrs["TEXCOORD_0"].integer()), &uv, &n) || n != 2) return *error = r.error, false;
            }
            if (uv.size() != count * 2) uv.assign(count * 2, 0.0);
            p.positions.assign(pos.begin(), pos.end());
            p.uvs.assign(uv.begin(), uv.end());
            if (attrs["JOINTS_0"].type != Json::Null && attrs["WEIGHTS_0"].type != Json::Null) {
                p.joints.assign(count * 8, 0);
                p.weights.assign(count * 8, 0.0f);
                for (int set = 0; set < 2; set++) {
                    std::string js = "JOINTS_" + std::to_string(set), ws = "WEIGHTS_" + std::to_string(set);
                    if (attrs[js.c_str()].type == Json::Null || attrs[ws.c_str()].type == Json::Null) continue;
                    std::vector<double> joints, weights;
                    int nj, nw;
                    if (!r.accessor(int(attrs[js.c_str()].integer()), &joints, &nj) || !r.accessor(int(attrs[ws.c_str()].integer()), &weights, &nw))
                        return *error = r.error, false;
                    if (nj != 4 || nw != 4 || joints.size() != count * 4 || weights.size() != count * 4)
                        return *error = T("у сетки разное число позиций и весов", "the mesh has different counts of positions and weights"), false;
                    for (size_t i = 0; i < count; i++)
                        for (int c = 0; c < 4; c++) {
                            p.joints[i * 8 + size_t(set * 4 + c)] = uint16_t(std::max(0.0, std::min(joints[i * 4 + size_t(c)], 65535.0)));
                            p.weights[i * 8 + size_t(set * 4 + c)] = float(std::max(0.0, weights[i * 4 + size_t(c)]));
                        }
                }
            }
            if (prim["indices"].type != Json::Null) {
                if (!r.accessor(int(prim["indices"].integer()), &idx, &n) || n != 1) return *error = r.error, false;
            } else {
                for (size_t i = 0; i < count; i++) idx.push_back(double(i));
            }
            if (idx.size() % 3) return *error = T("число индексов не кратно 3", "the index count is not a multiple of 3"), false;
            for (double i : idx) {
                if (i < 0 || i >= double(count)) return *error = T("индекс за концом вершин", "an index is past the last vertex"), false;
                p.indices.push_back(uint32_t(i));
            }
            if (prim["material"].type != Json::Null) p.material = int(prim["material"].integer());
            for (const char* a : {"POSITION", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0", "JOINTS_1", "WEIGHTS_1"})
                p.vertices += std::to_string(attrs[a].integer(-1)) + "/";
            mesh.primitives.push_back(p);
        }
        out->meshes.push_back(mesh);
    }

    const Json& textures = r.doc["textures"];
    for (const Json& m : r.doc["materials"].items) {
        Material mat;
        const Json& pbr = m["pbrMetallicRoughness"];
        if (pbr["baseColorFactor"].items.size() == 4)
            for (int i = 0; i < 4; i++) mat.color[i] = float(pbr["baseColorFactor"][size_t(i)].number);
        const Json& tex = pbr["baseColorTexture"];
        if (tex.type != Json::Null && tex["index"].integer() >= 0 && size_t(tex["index"].integer()) < textures.items.size()) {
            const Json& t = textures[size_t(tex["index"].integer())];
            const Json* source = &t["source"];
            if (source->type == Json::Null) source = &t["extensions"]["EXT_texture_webp"]["source"];
            if (source->type != Json::Null) mat.image = int(source->integer());
        }
        std::string mode = m["alphaMode"].str();
        mat.alpha = mode == "MASK" || mode == "BLEND";
        mat.doubleSided = m["doubleSided"].boolean;
        out->materials.push_back(mat);
    }
    for (const Json& image : r.doc["images"].items) {
        Image im;
        loadImage(r, image, &im);
        out->images.push_back(im);
    }

    const Json& vrm1 = r.doc["extensions"]["VRMC_vrm"]["humanoid"]["humanBones"];
    if (vrm1.type == Json::Object)
        for (const auto& bone : vrm1.fields)
            if (bone.second["node"].type != Json::Null) out->humanoid[bone.first] = int(bone.second["node"].integer());
    for (const Json& bone : r.doc["extensions"]["VRM"]["humanoid"]["humanBones"].items)
        if (bone["node"].type != Json::Null && !bone["bone"].str().empty()) out->humanoid[bone["bone"].str()] = int(bone["node"].integer());
    for (auto it = out->humanoid.begin(); it != out->humanoid.end();)
        it = it->second >= 0 && size_t(it->second) < out->nodes.size() ? std::next(it) : out->humanoid.erase(it);
    return true;
}

bool toModel(const Scene& scene, Model* out, std::string* error) {
    int skinIndex = -1;
    for (const Node& n : scene.nodes)
        if (n.mesh >= 0 && n.skin >= 0 && size_t(n.skin) < scene.skins.size() && size_t(n.mesh) < scene.meshes.size()) {
            skinIndex = n.skin;
            break;
        }
    if (skinIndex < 0) return *error = T("нет сетки со скелетом (узел с mesh и skin)", "no skinned mesh (a node with mesh and skin)"), false;
    const Skin& skin = scene.skins[size_t(skinIndex)];
    if (skin.joints.empty()) return *error = T("у скелета нет костей", "the skeleton has no bones"), false;
    out->bones.assign(skin.joints.size(), Bone());
    for (size_t b = 0; b < skin.joints.size(); b++) {
        const Node& node = scene.nodes[size_t(skin.joints[b])];
        out->bones[b].name = node.name;
        memcpy(out->bones[b].matrix, node.local, sizeof node.local);
        auto it = std::find(skin.joints.begin(), skin.joints.end(), node.parent);
        if (node.parent >= 0 && it != skin.joints.end()) out->bones[b].parent = int(it - skin.joints.begin());
    }
    out->inverseBind = skin.inverseBind;
    std::map<std::string, size_t> shared;
    for (const Node& node : scene.nodes) {
        if (node.mesh < 0 || node.skin != skinIndex || size_t(node.mesh) >= scene.meshes.size()) continue;
        for (const Primitive& prim : scene.meshes[size_t(node.mesh)].primitives) {
            if (prim.joints.empty())
                return *error = T("сетка не привязана к скелету (нет JOINTS_0/WEIGHTS_0)", "the mesh is not bound to the skeleton (no JOINTS_0/WEIGHTS_0)"), false;
            size_t count = prim.positions.size() / 3, base = out->vertexCount();
            auto known = shared.find(std::to_string(node.mesh) + ":" + prim.vertices);
            if (known != shared.end()) {
                base = known->second;
                count = 0;
            } else {
                shared[std::to_string(node.mesh) + ":" + prim.vertices] = base;
            }
            if (count) {
                out->positions.insert(out->positions.end(), prim.positions.begin(), prim.positions.end());
                out->uvs.insert(out->uvs.end(), prim.uvs.begin(), prim.uvs.end());
            }
            for (size_t i = 0; i < count; i++) {
                std::vector<std::pair<float, int>> w;
                for (int c = 0; c < 8; c++)
                    if (prim.weights[i * 8 + size_t(c)] > 0) w.push_back({prim.weights[i * 8 + size_t(c)], prim.joints[i * 8 + size_t(c)]});
                std::sort(w.rbegin(), w.rend());
                w.resize(4, {0.0f, 0});
                float sum = w[0].first + w[1].first + w[2].first + w[3].first;
                for (int c = 0; c < 4; c++) {
                    if (size_t(w[size_t(c)].second) >= out->bones.size())
                        return *error = T("вес вершины — на кость не из скелета", "a vertex weight refers to a bone outside the skeleton"), false;
                    out->joints.push_back(uint16_t(w[size_t(c)].second));
                    out->weights.push_back(sum > 0 ? w[size_t(c)].first / sum : (c == 0 ? 1.0f : 0.0f));
                }
            }
            Part part;
            part.firstIndex = uint32_t(out->indices.size());
            part.indexCount = uint32_t(prim.indices.size());
            for (uint32_t i : prim.indices) out->indices.push_back(uint32_t(base + i));
            if (prim.material >= 0 && size_t(prim.material) < scene.materials.size()) {
                const Material& m = scene.materials[size_t(prim.material)];
                part.alpha = m.alpha;
                part.doubleSided = m.doubleSided;
                if (m.image >= 0 && size_t(m.image) < scene.images.size())
                    part.image = scene.images[size_t(m.image)].bytes, part.imageMime = scene.images[size_t(m.image)].mime;
            }
            if (part.indexCount) out->parts.push_back(part);
        }
    }
    if (out->indices.empty()) return *error = T("в сетке нет треугольников", "the mesh has no triangles"), false;
    return true;
}

bool read(const std::string& path, Model* out, std::string* error) {
    Scene scene;
    return readScene(path, &scene, error) && toModel(scene, out, error);
}

namespace {

struct Writer {
    std::string bin;
    std::ostringstream views, accessors;
    int viewCount = 0, accessorCount = 0;

    int addView(const void* data, size_t size, int target) {
        while (bin.size() % 4) bin += '\0';
        size_t offset = bin.size();
        bin.append(static_cast<const char*>(data), size);
        views << (viewCount ? "," : "") << "{\"buffer\":0,\"byteOffset\":" << offset << ",\"byteLength\":" << size;
        if (target) views << ",\"target\":" << target;
        views << "}";
        return viewCount++;
    }

    int addAccessor(int view, int componentType, size_t count, const char* type, const std::string& extra = "") {
        accessors << (accessorCount ? "," : "") << "{\"bufferView\":" << view << ",\"componentType\":" << componentType
                  << ",\"count\":" << count << ",\"type\":\"" << type << "\"" << extra << "}";
        return accessorCount++;
    }
};

std::string numbers(const float* v, int n) {
    std::ostringstream s;
    s.precision(9);
    for (int i = 0; i < n; i++) s << (i ? "," : "") << v[i];
    return s.str();
}

}

bool write(const std::string& path, const Model& m, std::string* error) {
    Writer w;
    size_t n = m.vertexCount();
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (size_t i = 0; i < n; i++)
        for (int c = 0; c < 3; c++) lo[c] = std::min(lo[c], m.positions[i * 3 + size_t(c)]), hi[c] = std::max(hi[c], m.positions[i * 3 + size_t(c)]);
    int pos = w.addAccessor(w.addView(m.positions.data(), m.positions.size() * 4, 34962), 5126, n, "VEC3",
                            ",\"min\":[" + numbers(lo, 3) + "],\"max\":[" + numbers(hi, 3) + "]");
    int uv = w.addAccessor(w.addView(m.uvs.data(), m.uvs.size() * 4, 34962), 5126, n, "VEC2");
    int joints = w.addAccessor(w.addView(m.joints.data(), m.joints.size() * 2, 34962), 5123, n, "VEC4");
    int weights = w.addAccessor(w.addView(m.weights.data(), m.weights.size() * 4, 34962), 5126, n, "VEC4");
    int indexView = w.addView(m.indices.data(), m.indices.size() * 4, 34963);
    int ibm = w.addAccessor(w.addView(m.inverseBind.data(), m.inverseBind.size() * 4, 0), 5126, m.bones.size(), "MAT4");
    std::vector<Part> parts = m.parts;
    if (parts.empty()) parts.push_back(Part()), parts[0].indexCount = uint32_t(m.indices.size());
    std::vector<int> partIndices, partImage;
    for (const Part& p : parts) {
        partIndices.push_back(w.addAccessor(indexView, 5125, p.indexCount, "SCALAR", ",\"byteOffset\":" + std::to_string(p.firstIndex * 4)));
        partImage.push_back(p.image.empty() ? -1 : w.addView(p.image.data(), p.image.size(), 0));
    }

    std::ostringstream j;
    j << "{\"asset\":{\"version\":\"2.0\",\"generator\":\"medecomp\"},\"scene\":0";
    std::ostringstream roots;
    roots << "0";
    j << ",\"nodes\":[{\"name\":\"heroine\",\"mesh\":0,\"skin\":0}";
    for (size_t b = 0; b < m.bones.size(); b++) {
        j << ",{\"name\":" << port::online::jsonString(m.bones[b].name) << ",\"matrix\":[" << numbers(m.bones[b].matrix, 16) << "]";
        std::ostringstream kids;
        for (size_t c = 0; c < m.bones.size(); c++)
            if (m.bones[c].parent == int(b)) kids << (kids.tellp() ? "," : "") << c + 1;
        if (kids.tellp()) j << ",\"children\":[" << kids.str() << "]";
        j << "}";
        if (m.bones[b].parent < 0) roots << "," << b + 1;
    }
    j << "],\"scenes\":[{\"nodes\":[" << roots.str() << "]}]";
    j << ",\"skins\":[{\"joints\":[";
    for (size_t b = 0; b < m.bones.size(); b++) j << (b ? "," : "") << b + 1;
    j << "],\"inverseBindMatrices\":" << ibm << "}]";
    j << ",\"meshes\":[{\"name\":\"heroine\",\"primitives\":[";
    int materials = 0;
    for (size_t p = 0; p < parts.size(); p++) {
        j << (p ? "," : "") << "{\"attributes\":{\"POSITION\":" << pos << ",\"TEXCOORD_0\":" << uv << ",\"JOINTS_0\":" << joints
          << ",\"WEIGHTS_0\":" << weights << "},\"indices\":" << partIndices[p];
        if (partImage[p] >= 0) j << ",\"material\":" << materials++;
        j << "}";
    }
    j << "]}]";
    if (materials) {
        std::ostringstream mats, texs, imgs;
        int k = 0;
        for (size_t p = 0; p < parts.size(); p++) {
            if (partImage[p] < 0) continue;
            mats << (k ? "," : "") << "{\"name\":\"part" << k << "\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":" << k
                 << "},\"metallicFactor\":0,\"roughnessFactor\":1},\"extensions\":{\"KHR_materials_unlit\":{}}"
                 << (parts[p].alpha ? ",\"alphaMode\":\"MASK\"" : "") << (parts[p].doubleSided ? ",\"doubleSided\":true" : "") << "}";
            texs << (k ? "," : "") << "{\"sampler\":0,\"source\":" << k << "}";
            imgs << (k ? "," : "") << "{\"bufferView\":" << partImage[p] << ",\"mimeType\":\""
                 << (parts[p].imageMime.empty() ? "image/png" : parts[p].imageMime) << "\"}";
            k++;
        }
        j << ",\"materials\":[" << mats.str() << "],\"extensionsUsed\":[\"KHR_materials_unlit\"]"
          << ",\"samplers\":[{\"magFilter\":9729,\"minFilter\":9987}],\"textures\":[" << texs.str() << "],\"images\":[" << imgs.str() << "]";
    }
    j << ",\"bufferViews\":[" << w.views.str() << "],\"accessors\":[" << w.accessors.str() << "]";
    while (w.bin.size() % 4) w.bin += '\0';
    j << ",\"buffers\":[{\"byteLength\":" << w.bin.size() << "}]}";
    std::string json = j.str();
    while (json.size() % 4) json += ' ';

    std::string glb;
    auto put = [&](uint32_t v) { glb.append(reinterpret_cast<const char*>(&v), 4); };
    put(kMagic);
    put(2);
    put(uint32_t(12 + 8 + json.size() + 8 + w.bin.size()));
    put(uint32_t(json.size()));
    put(kChunkJson);
    glb += json;
    put(uint32_t(w.bin.size()));
    put(kChunkBin);
    glb += w.bin;
    std::ofstream out(path.c_str(), std::ios::binary);
    out.write(glb.data(), std::streamsize(glb.size()));
    if (!out) return *error = T("не записать ", "cannot write ") + path, false;
    return true;
}

}
}
}
