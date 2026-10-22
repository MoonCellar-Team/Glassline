#pragma once

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

namespace port {
namespace platform {
namespace gltf {

struct Bone {
    std::string name;
    int parent = -1;
    float matrix[16];
};

struct Part {
    uint32_t firstIndex = 0, indexCount = 0;
    std::string image;
    std::string imageMime;
    bool alpha = false;
    bool doubleSided = false;
};

struct Model {
    std::vector<float> positions;
    std::vector<float> uvs;
    std::vector<uint16_t> joints;
    std::vector<float> weights;
    std::vector<uint32_t> indices;
    std::vector<Part> parts;
    std::vector<Bone> bones;
    std::vector<float> inverseBind;

    size_t vertexCount() const { return positions.size() / 3; }
    bool hasImage() const {
        for (const Part& p : parts)
            if (!p.image.empty()) return true;
        return false;
    }
};

struct Node {
    std::string name;
    int parent = -1;
    float local[16], world[16];
    int mesh = -1, skin = -1;
};

struct Skin {
    std::vector<int> joints;
    std::vector<float> inverseBind;
};

struct Primitive {
    std::vector<float> positions, uvs;
    std::vector<uint16_t> joints;
    std::vector<float> weights;
    std::vector<uint32_t> indices;
    int material = -1;
    std::string vertices;
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
};

struct Material {
    int image = -1;
    float color[4] = {1, 1, 1, 1};
    bool alpha = false, doubleSided = false;
};

struct Image {
    std::string bytes, mime;
};

struct Scene {
    std::vector<Node> nodes;
    std::vector<Skin> skins;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Image> images;
    std::map<std::string, int> humanoid;
};

void setRussian(bool russian);
std::string T(const char* ru, const char* en);

bool readScene(const std::string& path, Scene* out, std::string* error);
bool toModel(const Scene& scene, Model* out, std::string* error);
bool read(const std::string& path, Model* out, std::string* error);
bool write(const std::string& path, const Model& model, std::string* error);

}
}
}
