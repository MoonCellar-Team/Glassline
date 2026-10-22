#pragma once

#include <string>

namespace port {
namespace platform {
namespace settings {

std::string get(const char* key, const std::string& fallback = std::string());
void set(const char* key, const std::string& value);
void erase(const char* key);
bool save();
std::string path();

}
}
}
