#pragma once

#include <string>
#include <vector>

namespace m3g {
class Image2D;
class Node;
}

namespace port {
namespace platform {
namespace skins {

struct Choice {
    std::string key;
    std::string ru, en;
    bool custom = false;
};

std::vector<Choice> list();
std::string selected();
void select(const std::string& key);
void rescan();
std::string folder();

std::string previewFile(const Choice& c);
std::string previewStamp(const std::string& game);
bool combinePreview(const std::string& black, const std::string& white, const std::string& out);

struct Look {
    int model = -1;
    int keep = 102;
    int textureModel = -1;
    std::string texture;
    std::string model3d;
};
Look lookOf(const std::string& key);
void dress(m3g::Node* root, const Look& look);
m3g::Image2D* firstTextureImage(m3g::Node* node);
m3g::Image2D* modelTexture(int model);

std::string baseKeyOf(const std::string& key);
bool plainFaith(const Look& look);
void recolorShirt(m3g::Node* root, const Look& look);

}
}
}
