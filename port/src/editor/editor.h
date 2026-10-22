#pragma once

#include <string>
#include <vector>

namespace port {
namespace editor {

int run(const std::string& id, const char* argv0);
long long launch(const std::vector<std::string>& args, const char* argv0 = "");
#ifdef __ANDROID__
bool takeTestLevel(std::string* id);
bool takeServiceRun(std::vector<std::string>* env, std::string* level);
bool takeResume(std::string* id);
#endif

}
}
