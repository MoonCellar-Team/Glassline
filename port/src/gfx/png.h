#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace port {
namespace gfx {

struct Image {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;
};

bool decodePNG(const uint8_t* data, size_t size, Image& out, bool premultiplied = true);
bool writePNG(const std::string& path, int width, int height, const uint8_t* rgba);
bool encodePNG(int width, int height, const uint8_t* rgba, std::string* png);

}
}
