#pragma once

#include <stdint.h>

#include <string>
#include <vector>

namespace m3g {
class Image2D;
}

namespace port {
namespace gfx {

bool imageRGBA(m3g::Image2D* image, std::vector<uint8_t>* out, const char** why);

uint64_t textureHash(const void* data, int size, int width, int height, unsigned format);
bool decodePVRTCTexture(unsigned format, int width, int height, const void* data, int size, std::vector<uint8_t>* rgba);

bool texturePackSetting();
void setTexturePackSetting(bool on);
std::string texturesDir();
int dumpTextures();

}
}
