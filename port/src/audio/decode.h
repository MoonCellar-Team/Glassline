#pragma once

#include <vector>

namespace port {
namespace audio {

bool decodeToPcm16(const std::vector<unsigned char>& file, std::vector<unsigned char>* pcm, int* rate,
                   int* channels);

}
}
