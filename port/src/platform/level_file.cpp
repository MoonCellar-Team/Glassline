#include "level_file.h"

#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>

#include <vector>

#include "../ea/json.h"
#include "levels.h"
#include "port/files.h"

namespace port {
namespace platform {

const char* const kLevelFileExtension = ".glevel";

namespace {

using port::online::Json;

const size_t kMaxMap = 256 * 1024;
const size_t kMaxFile = 1024 * 1024;
const size_t kMaxName = 64;

std::string levelsDir() { return port::files::dataDir() + "/levels"; }

bool readJson(const std::string& path, Json* out) {
    std::vector<unsigned char> text;
    return port::files::read(path, &text) && port::online::parseJson(std::string(text.begin(), text.end()), out) &&
           out->type == Json::Object;
}

bool writeFile(const std::string& path, const std::string& data) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

bool chapterName(const std::string& s) {
    if (s.empty() || s.size() > 40) return false;
    for (char c : s)
        if (!islower((unsigned char)c) && !isdigit((unsigned char)c) && c != '_') return false;
    return true;
}

std::string freeId(const std::string& wanted) {
    std::string base;
    for (char c : wanted) {
        const unsigned char u = (unsigned char)c;
        const char k = isalnum(u) && u < 0x80 ? char(tolower(u)) : '-';
        if (k != '-' || (!base.empty() && base.back() != '-')) base += k;
    }
    while (!base.empty() && base.back() == '-') base.pop_back();
    if (base.size() > 40) base.resize(40);
    if (base.empty()) base = "imported";
    std::string id = base;
    for (int n = 2; port::files::stat(levelsDir() + "/" + id).exists; n++) id = base + "-" + std::to_string(n);
    return id;
}

std::string levelName(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    std::string name = a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
    if (name.size() > kMaxName) {
        size_t cut = kMaxName;
        while (cut > 0 && ((unsigned char)name[cut] & 0xc0) == 0x80) cut--;
        name.resize(cut);
    }
    return name;
}

}

bool levelExportable(const std::string& id) {
    Json info;
    if (!readJson(levelsDir() + "/" + id + "/level.json", &info)) return false;
    const std::string origin = info["origin"].str();
    return origin == "empty" || origin == "uploaded";
}

bool exportLevelFile(const std::string& id, const std::string& path, std::string* error) {
    Json info;
    std::vector<unsigned char> map;
    if (!readJson(levelsDir() + "/" + id + "/level.json", &info) || !port::files::read(levelsDir() + "/" + id + "/level.bin", &map))
        return *error = "нет файлов уровня", false;
    if (!levelExportable(id)) return *error = "экспортировать можно только свои уровни, собранные с нуля", false;
    using port::online::jsonString;
    const bool bags = info["mode"].str() == "bags";
    std::string s = "{\n  \"glassline\": \"level\",\n  \"format\": 1,\n  \"id\": " + jsonString(id) +
                    ",\n  \"name\": " + jsonString(info["name"].str(id)) + ",\n  \"author\": " + jsonString(info["author"].str()) +
                    ",\n  \"palette\": " + jsonString(info["palette"].str()) +
                    ",\n  \"description\": " + jsonString(info["description"].str()) + ",\n  \"mode\": \"" +
                    (bags ? "bags" : "race") + "\",\n  \"timeLimit\": " +
                    std::to_string(bags ? info["timeLimit"].integer(kBagTimeLimitDefault) : 0);
    if (!info["supports"].str().empty()) s += ",\n  \"supports\": " + jsonString(info["supports"].str());
    s += ",\n  \"map\": \"" + port::online::base64Encode(map.data(), map.size()) + "\"\n}\n";
    if (!writeFile(path, s)) return *error = "не записать " + path, false;
    return true;
}

bool importLevelFile(const std::string& path, std::string* id, std::string* name, std::string* error) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return *error = "нет файла " + path, false;
    if (size_t(st.st_size) > kMaxFile) return *error = "файл слишком большой для уровня", false;
    Json j;
    if (!readJson(path, &j) || j["glassline"].str() != "level") return *error = "это не файл уровня", false;
    if (j["format"].integer() > 1) return *error = "файл из более новой версии порта — обновите порт", false;
    const std::string palette = j["palette"].str();
    if (!chapterName(palette)) return *error = "в файле нет главы-основы", false;
    std::string map;
    if (!port::online::base64Decode(j["map"].str(), &map) || map.empty()) return *error = "в файле нет карты", false;
    if (map.size() > kMaxMap) return *error = "карта больше 256 КиБ", false;
    const bool bags = j["mode"].str() == "bags";
    int timeLimit = 0;
    if (bags) {
        timeLimit = kBagTimeLimitDefault;
        for (int limit : kBagTimeLimits)
            if (j["timeLimit"].integer() == limit) timeLimit = limit;
    }
    const std::string check = checkPlayerMap(std::vector<unsigned char>(map.begin(), map.end()), bags);
    if (!check.empty()) return *error = "карта не годится: " + check, false;
    *name = levelName(j["name"].str());
    *id = freeId(j["id"].str(*name));
    if (name->empty()) *name = *id;
    const std::string supports = j["supports"].str(), dir = levelsDir() + "/" + *id;
    using port::online::jsonString;
    std::string info = "{\n  \"name\": " + jsonString(*name) + ",\n  \"author\": " + jsonString(levelName(j["author"].str())) +
                       ",\n  \"palette\": " + jsonString(palette) + ",\n  \"version\": 1,\n  \"origin\": \"empty\",\n  \"description\": " +
                       jsonString(j["description"].str()) + ",\n  \"mode\": \"" + (bags ? "bags" : "race") + "\"";
    if (bags) info += ",\n  \"timeLimit\": " + std::to_string(timeLimit);
    if (supports == "visible" || supports == "hidden") info += ",\n  \"supports\": \"" + supports + "\"";
    info += "\n}\n";
    mkdir(levelsDir().c_str(), 0755);
    if (mkdir(dir.c_str(), 0755) != 0 || !writeFile(dir + "/level.bin", map) || !writeFile(dir + "/level.json", info))
        return *error = "не записать " + dir, false;
    return true;
}

}
}
