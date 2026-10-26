#pragma once

#include <string>

namespace port {
namespace platform {
namespace dialog {

enum Kind { Ipa, MusicFolder, Png, Gltf, LevelFile };

void pick(Kind kind, const std::string& current, const std::string& title);
bool busy();
bool take(Kind* kind, std::string* path, bool* noDialog);
bool noDialog();
int copyProgress();
bool copyFailed();

}
}
}
