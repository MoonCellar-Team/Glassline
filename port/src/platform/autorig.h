#pragma once

#include <string>
#include <vector>

#include "gltf.h"

namespace port {
namespace platform {
namespace autorig {

struct Report {
    bool bySkeleton = false;
    std::string rig;
    std::vector<std::string> missing;
    int mapped = 0, merged = 0;
    int transferred = 0;
    bool turned = false;
    bool downscaled = false;
    int parts = 0;
};

bool fit(const gltf::Scene& scene, const gltf::Model& faith, gltf::Model* out, Report* report, std::string* error);

}
}
}
