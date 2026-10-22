#pragma once

#include <string>
#include <vector>

#include "gltf.h"

namespace m3g {
class Image2D;
class Node;
}

namespace port {
namespace platform {
namespace heroine {

const char* boneName(int index);
int boneCount();
bool faithTemplate(m3g::Node* figure, const std::string& png, gltf::Model* out, std::string* error);
bool checkModel(const gltf::Model& model, std::vector<int>* boneMap, std::string* error);
bool wearModel(m3g::Node* figure, const gltf::Model& model, m3g::Image2D* image, std::string* error);
void hideOthers(m3g::Node* root, m3g::Node* figure);
bool torsoMask(m3g::Node* figure, int w, int h, std::vector<uint8_t>* mask);

}
}
}
