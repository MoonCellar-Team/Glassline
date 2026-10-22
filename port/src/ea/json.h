#pragma once

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

namespace port {
namespace online {

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::vector<Json> items;
    std::vector<std::pair<std::string, Json>> fields;

    const Json& operator[](const char* key) const;
    const Json& operator[](size_t index) const;
    std::string str(const std::string& fallback = std::string()) const;
    long long integer(long long fallback = 0) const;
};

bool parseJson(const std::string& text, Json* out);
std::string jsonString(const std::string& text);

std::string base64Encode(const void* data, size_t size);
bool base64Decode(const std::string& text, std::string* out);

std::string errorOf(int status, const std::string& body, const std::string& transportError);

}
}
