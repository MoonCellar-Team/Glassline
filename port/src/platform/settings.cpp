#include "settings.h"

#include <stdio.h>

#include <algorithm>
#include <map>
#include <vector>

#include "port/files.h"

namespace {

std::map<std::string, std::string> s_values;
std::vector<std::string> s_order;
bool s_loaded;

void skipSpace(const std::string& s, size_t* i) {
    while (*i < s.size() && (s[*i] == ' ' || s[*i] == '\t' || s[*i] == '\n' || s[*i] == '\r')) ++*i;
}

bool readString(const std::string& s, size_t* i, std::string* out) {
    if (*i >= s.size() || s[*i] != '"') return false;
    out->clear();
    for (++*i; *i < s.size(); ++*i) {
        char c = s[*i];
        if (c == '"') {
            ++*i;
            return true;
        }
        if (c != '\\') {
            *out += c;
            continue;
        }
        if (++*i >= s.size()) return false;
        char e = s[*i];
        if (e == 'n') *out += '\n';
        else if (e == 't') *out += '\t';
        else if (e == 'r') *out += '\r';
        else if (e == 'b') *out += '\b';
        else if (e == 'f') *out += '\f';
        else if (e == 'u') {
            if (*i + 4 >= s.size()) return false;
            unsigned cp = unsigned(strtoul(s.substr(*i + 1, 4).c_str(), nullptr, 16));
            *i += 4;
            if (cp >= 0xD800 && cp < 0xDC00 && *i + 6 < s.size() && s[*i + 1] == '\\' && s[*i + 2] == 'u') {
                unsigned lo = unsigned(strtoul(s.substr(*i + 3, 4).c_str(), nullptr, 16));
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                *i += 6;
            }
            if (cp < 0x80) *out += char(cp);
            else if (cp < 0x800) { *out += char(0xC0 | cp >> 6); *out += char(0x80 | (cp & 0x3F)); }
            else if (cp < 0x10000) {
                *out += char(0xE0 | cp >> 12); *out += char(0x80 | (cp >> 6 & 0x3F)); *out += char(0x80 | (cp & 0x3F));
            } else {
                *out += char(0xF0 | cp >> 18); *out += char(0x80 | (cp >> 12 & 0x3F));
                *out += char(0x80 | (cp >> 6 & 0x3F)); *out += char(0x80 | (cp & 0x3F));
            }
        } else *out += e;
    }
    return false;
}

bool readValue(const std::string& s, size_t* i, std::string* raw) {
    size_t start = *i;
    if (*i < s.size() && s[*i] == '"') {
        std::string dummy;
        if (!readString(s, i, &dummy)) return false;
    } else if (*i < s.size() && (s[*i] == '{' || s[*i] == '[')) {
        int depth = 0;
        for (; *i < s.size(); ++*i) {
            char c = s[*i];
            if (c == '"') {
                std::string dummy;
                if (!readString(s, i, &dummy)) return false;
                --*i;
            } else if (c == '{' || c == '[') {
                depth++;
            } else if ((c == '}' || c == ']') && --depth == 0) {
                ++*i;
                break;
            }
        }
    } else {
        while (*i < s.size() && s[*i] != ',' && s[*i] != '}' && s[*i] != ' ' && s[*i] != '\n' && s[*i] != '\r' &&
               s[*i] != '\t')
            ++*i;
    }
    *raw = s.substr(start, *i - start);
    return *i > start;
}

std::string quote(const std::string& v) {
    std::string out = "\"";
    for (unsigned char c : v) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c == '\r') out += "\\r";
        else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", c);
            out += buf;
        } else out += char(c);
    }
    return out + "\"";
}

void load() {
    if (s_loaded) return;
    s_loaded = true;
    std::vector<unsigned char> bytes;
    if (!port::files::read(port::platform::settings::path(), &bytes)) return;
    std::string s(bytes.begin(), bytes.end());
    size_t i = 0;
    skipSpace(s, &i);
    if (i >= s.size() || s[i] != '{') return;
    ++i;
    while (true) {
        skipSpace(s, &i);
        std::string key, raw;
        if (!readString(s, &i, &key)) return;
        skipSpace(s, &i);
        if (i >= s.size() || s[i] != ':') return;
        ++i;
        skipSpace(s, &i);
        if (!readValue(s, &i, &raw)) return;
        if (!s_values.count(key)) s_order.push_back(key);
        s_values[key] = raw;
        skipSpace(s, &i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        return;
    }
}

}

namespace port {
namespace platform {
namespace settings {

std::string path() { return port::files::dataDir() + "/settings.json"; }

std::string get(const char* key, const std::string& fallback) {
    load();
    auto it = s_values.find(key);
    if (it == s_values.end()) return fallback;
    size_t i = 0;
    std::string out;
    return readString(it->second, &i, &out) ? out : fallback;
}

void set(const char* key, const std::string& value) {
    load();
    if (!s_values.count(key)) s_order.push_back(key);
    s_values[key] = quote(value);
}

void erase(const char* key) {
    load();
    if (!s_values.erase(key)) return;
    s_order.erase(std::remove(s_order.begin(), s_order.end(), std::string(key)), s_order.end());
}

bool save() {
    load();
    std::string out = "{\n";
    for (size_t k = 0; k < s_order.size(); k++)
        out += "  " + quote(s_order[k]) + ": " + s_values[s_order[k]] + (k + 1 < s_order.size() ? ",\n" : "\n");
    out += "}\n";
    std::string tmp = path() + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    ok = fclose(f) == 0 && ok;
    return ok && port::files::replaceFile(tmp, path());
}

}
}
}
