#pragma once

#include <string>

namespace port {
namespace platform {

extern const char* const kLevelFileExtension;
bool levelExportable(const std::string& id);
bool exportLevelFile(const std::string& id, const std::string& path, std::string* error);
bool importLevelFile(const std::string& path, std::string* id, std::string* name, std::string* error);

}
}
