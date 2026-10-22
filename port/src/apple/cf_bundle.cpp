#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "port/cf.h"
#include "port/files.h"

typedef long CFIndex;

using port::cf::Object;

namespace {

bool exists(const std::string& p, bool* dir = nullptr) {
    port::files::Info info = port::files::stat(p);
    if (dir) *dir = info.dir;
    return info.exists;
}

struct Array : Object {
    std::vector<const void*> items;
    Array();
};
void destroyArray(Object* o) {
    for (const void* v : static_cast<Array*>(o)->items) CFRelease(v);
    delete static_cast<Array*>(o);
}
const port::cf::Class kArrayClass = {"CFArray", destroyArray, nullptr};
Array::Array() : Object(&kArrayClass) {}

struct URL : Object {
    std::string path;
    bool directory;
    URL(const std::string& p, bool dir);
};
void destroyURL(Object* o) { delete static_cast<URL*>(o); }
std::u16string describeURL(const Object* o) {
    std::string s = "file://" + static_cast<const URL*>(o)->path;
    return port::cf::fromUtf8(s.data(), s.size());
}
const port::cf::Class kURLClass = {"CFURL", destroyURL, describeURL};
URL::URL(const std::string& p, bool dir) : Object(&kURLClass), path(p), directory(dir) {}

struct Dictionary : Object {
    std::vector<std::pair<const void*, const void*>> items;
    Dictionary();
};
void destroyDictionary(Object* o) {
    for (auto& kv : static_cast<Dictionary*>(o)->items) { CFRelease(kv.first); CFRelease(kv.second); }
    delete static_cast<Dictionary*>(o);
}
const port::cf::Class kDictionaryClass = {"CFDictionary", destroyDictionary, nullptr};
Dictionary::Dictionary() : Object(&kDictionaryClass) {}

struct Number : Object {
    long long value;
    explicit Number(long long v);
};
void destroyNumber(Object* o) { delete static_cast<Number*>(o); }
std::u16string describeNumber(const Object* o) {
    std::string s = std::to_string(static_cast<const Number*>(o)->value);
    return port::cf::fromUtf8(s.data(), s.size());
}
const port::cf::Class kNumberClass = {"CFNumber", destroyNumber, describeNumber};
Number::Number(long long v) : Object(&kNumberClass), value(v) {}

struct Locale : Object {
    std::string language;
    explicit Locale(const std::string& l);
};
void destroyLocale(Object* o) { delete static_cast<Locale*>(o); }
const port::cf::Class kLocaleClass = {"CFLocale", destroyLocale, nullptr};
Locale::Locale(const std::string& l) : Object(&kLocaleClass), language(l) {}

void neverDestroy(Object*) {}
const port::cf::Class kBundleClass = {"CFBundle", neverDestroy, nullptr};
struct Bundle : Object {
    Bundle() : Object(&kBundleClass) {}
};

struct Plist {
    std::vector<unsigned char> b;
    unsigned offSize = 0, refSize = 0;
    uint64_t count = 0, top = 0, table = 0;

    uint64_t be(size_t pos, unsigned n) const {
        uint64_t v = 0;
        for (unsigned i = 0; i < n && pos + i < b.size(); i++) v = v << 8 | b[pos + i];
        return v;
    }
    bool load(const std::string& path) {
        if (!port::files::read(path, &b)) return false;
        if (b.size() < 40 || memcmp(b.data(), "bplist00", 8) != 0) return false;
        size_t t = b.size() - 32;
        offSize = b[t + 6]; refSize = b[t + 7];
        count = be(t + 8, 8); top = be(t + 16, 8); table = be(t + 24, 8);
        return true;
    }
    size_t offset(uint64_t obj) const { return static_cast<size_t>(be(table + obj * offSize, offSize)); }
    uint64_t length(size_t& pos) const {
        unsigned low = b[pos] & 0x0F;
        pos++;
        if (low != 0x0F) return low;
        unsigned n = 1u << (b[pos] & 0x0F);
        uint64_t v = be(pos + 1, n);
        pos += 1 + n;
        return v;
    }
    const void* read(uint64_t obj, int depth = 0) const {
        if (obj >= count || depth > 32) return nullptr;
        size_t pos = offset(obj);
        if (pos >= b.size()) return nullptr;
        unsigned type = b[pos] >> 4;
        switch (type) {
        case 0x0:
            if (b[pos] == 0x08 || b[pos] == 0x09) return new Number(b[pos] == 0x09);
            return nullptr;
        case 0x1: return new Number(static_cast<long long>(be(pos + 1, 1u << (b[pos] & 0x0F))));
        case 0x5: {
            uint64_t n = length(pos);
            return port::cf::makeString(std::string(reinterpret_cast<const char*>(&b[pos]), n));
        }
        case 0x6: {
            uint64_t n = length(pos);
            std::u16string s;
            for (uint64_t i = 0; i < n; i++) s += static_cast<char16_t>(be(pos + 2 * i, 2));
            return port::cf::makeString(s);
        }
        case 0xA: {
            uint64_t n = length(pos);
            Array* a = new Array();
            for (uint64_t i = 0; i < n; i++)
                if (const void* v = read(be(pos + i * refSize, refSize), depth + 1)) a->items.push_back(v);
            return a;
        }
        case 0xD: {
            uint64_t n = length(pos);
            Dictionary* d = new Dictionary();
            for (uint64_t i = 0; i < n; i++) {
                const void* k = read(be(pos + i * refSize, refSize), depth + 1);
                const void* v = read(be(pos + (n + i) * refSize, refSize), depth + 1);
                if (k && v) d->items.push_back({k, v});
                else { CFRelease(k); CFRelease(v); }
            }
            return d;
        }
        default: return nullptr;
        }
    }
};

const Dictionary* infoDictionary() {
    static const Dictionary* info;
    static std::string loadedFrom;
    if (!info || loadedFrom != port::files::gameSource()) {
        loadedFrom = port::files::gameSource();
        Plist p;
        const void* d = p.load(port::cf::bundlePath() + "/Info.plist") ? p.read(p.top) : nullptr;
        info = d && static_cast<const Object*>(d)->isa == &kDictionaryClass ? static_cast<const Dictionary*>(d)
                                                                           : new Dictionary();
    }
    return info;
}

const void* dictionaryGet(const void* dict, const void* key) {
    if (!dict || static_cast<const Object*>(dict)->isa != &kDictionaryClass) return nullptr;
    std::u16string k = port::cf::text(key);
    for (auto& kv : static_cast<const Dictionary*>(dict)->items)
        if (port::cf::text(kv.first) == k) return kv.second;
    return nullptr;
}

std::vector<std::string> preferredLanguages() {
    std::vector<std::string> out;
    auto add = [&](std::string l) {
        size_t cut = l.find_first_of(".@");
        if (cut != std::string::npos) l.resize(cut);
        if (l.empty() || l == "C" || l == "POSIX") return;
        std::string base = l.substr(0, l.find('_'));
        for (const std::string& x : {l, base})
            if (std::find(out.begin(), out.end(), x) == out.end()) out.push_back(x);
    };
    for (const char* var : {"MEDECOMP_LANG", "LANGUAGE", "LC_ALL", "LANG"}) {
        const char* v = getenv(var);
        if (!v) continue;
        std::string s = v;
        size_t start = 0;
        while (start <= s.size()) {
            size_t end = s.find(':', start);
            add(s.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    if (std::find(out.begin(), out.end(), "en") == out.end()) out.push_back("en");
    return out;
}

}

namespace port {
namespace cf {

const std::string& bundlePath() { return port::files::bundle(); }

std::string infoString(const char* key) {
    const void* name = makeString(std::string(key));
    const void* v = dictionaryGet(infoDictionary(), name);
    CFRelease(name);
    return v && isString(v) ? utf8(v) : std::string();
}

}
}

extern "C" int __CFConstantStringClassReference[32];

namespace {
struct ConstString {
    const void* isa;
    int32_t flags;
    const char* str;
    long length;
};
#define CONST_STRING(name, text) const ConstString name = {__CFConstantStringClassReference, 0x07C8, text, sizeof(text) - 1}
CONST_STRING(s_bundleVersion, "CFBundleVersion");
CONST_STRING(s_languageCode, "kCFLocaleLanguageCode");
CONST_STRING(s_commonModes, "kCFRunLoopCommonModes");
}

extern "C" {

extern const void* const kCFBundleVersionKey = &s_bundleVersion;
extern const void* const kCFLocaleLanguageCode = &s_languageCode;
extern const void* const kCFRunLoopCommonModes = &s_commonModes;

CFIndex CFArrayGetCount(const void* a) {
    return a ? static_cast<CFIndex>(static_cast<const Array*>(a)->items.size()) : 0;
}

const void* CFArrayGetValueAtIndex(const void* a, CFIndex i) {
    const Array* arr = static_cast<const Array*>(a);
    return arr && i >= 0 && static_cast<size_t>(i) < arr->items.size() ? arr->items[i] : nullptr;
}

const void* CFBundleGetMainBundle(void) {
    static Bundle bundle;
    return &bundle;
}

const void* CFBundleCopyBundleURL(const void*) { return new URL(port::cf::bundlePath(), true); }

const void* CFBundleCopyBundleLocalizations(const void*) {
    Array* a = new Array();
    std::vector<std::string> names, ordered;
    for (const std::string& n : port::files::list(port::cf::bundlePath()))
        if (n.size() > 6 && n.compare(n.size() - 6, 6, ".lproj") == 0) names.push_back(n.substr(0, n.size() - 6));
    std::sort(names.begin(), names.end());
    const void* key = port::cf::makeString(std::string("CFBundleLocalizations"));
    const void* listed = dictionaryGet(infoDictionary(), key);
    CFRelease(key);
    if (listed && static_cast<const Object*>(listed)->isa == &kArrayClass)
        for (const void* item : static_cast<const Array*>(listed)->items)
            if (port::cf::isString(item) && std::count(names.begin(), names.end(), port::cf::utf8(item)) &&
                !std::count(ordered.begin(), ordered.end(), port::cf::utf8(item)))
                ordered.push_back(port::cf::utf8(item));
    for (const std::string& n : names)
        if (!std::count(ordered.begin(), ordered.end(), n)) ordered.push_back(n);
    for (const std::string& n : ordered) a->items.push_back(port::cf::makeString(n));
    return a;
}

const void* CFBundleCopyResourceURL(const void*, const void* name, const void* type, const void* subdir) {
    std::string file = port::cf::utf8(name);
    if (type) file += "." + port::cf::utf8(type);
    std::string sub = subdir ? port::cf::utf8(subdir) : std::string();
    std::string base = port::cf::bundlePath();
    std::vector<std::string> dirs = {base};
    for (const std::string& l : preferredLanguages()) dirs.push_back(base + "/" + l + ".lproj");
    for (const std::string& d : dirs) {
        std::string path = d + (sub.empty() ? "" : "/" + sub) + "/" + file;
        bool dir = false;
        if (exists(path, &dir)) return new URL(path, dir);
    }
    return nullptr;
}

const void* CFBundleGetValueForInfoDictionaryKey(const void*, const void* key) {
    return dictionaryGet(infoDictionary(), key);
}

const void* CFURLCreateCopyDeletingLastPathComponent(const void*, const void* url) {
    std::string p = url ? static_cast<const URL*>(url)->path : std::string();
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    size_t slash = p.rfind('/');
    p = slash == std::string::npos ? std::string(".") : slash == 0 ? std::string("/") : p.substr(0, slash);
    return new URL(p, true);
}

const void* CFURLCreateCopyAppendingPathComponent(const void*, const void* url, const void* component, bool dir) {
    std::string p = url ? static_cast<const URL*>(url)->path : std::string(".");
    if (p.empty() || p.back() != '/') p += '/';
    return new URL(p + port::cf::utf8(component), dir);
}

bool CFURLGetFileSystemRepresentation(const void* url, bool, char* buffer, CFIndex max) {
    if (!url || !buffer || max <= 0) return false;
    const std::string& p = static_cast<const URL*>(url)->path;
    if (static_cast<CFIndex>(p.size()) + 1 > max) return false;
    memcpy(buffer, p.c_str(), p.size() + 1);
    return true;
}

const void* CFLocaleCopyPreferredLanguages(void) {
    Array* a = new Array();
    for (const std::string& l : preferredLanguages()) a->items.push_back(port::cf::makeString(l));
    return a;
}

const void* CFLocaleCopyCurrent(void) { return new Locale(preferredLanguages().front()); }

const void* CFLocaleGetValue(const void* locale, const void* key) {
    if (!locale || port::cf::text(key) != port::cf::text(kCFLocaleLanguageCode)) return nullptr;
    static std::map<std::string, const void*> cache;
    const std::string& l = static_cast<const Locale*>(locale)->language;
    std::string code = l.substr(0, l.find('_'));
    const void*& s = cache[code];
    if (!s) s = port::cf::makeString(code);
    return s;
}

}

namespace port {
namespace cf {
const void* makeDictionary(const std::vector<std::pair<const void*, const void*>>& items) {
    Dictionary* d = new Dictionary();
    d->items = items;
    return d;
}
const void* dictionaryValue(const void* dict, const void* key) { return dictionaryGet(dict, key); }
const void* makeNumber(long long v) { return new Number(v); }
long long numberValue(const void* n) {
    return n && static_cast<const Object*>(n)->isa == &kNumberClass ? static_cast<const Number*>(n)->value : 0;
}
const void* makeFileURL(const std::string& path) {
    bool dir = false;
    exists(path, &dir);
    return new URL(path, dir);
}
std::string urlPath(const void* url) {
    return url && static_cast<const Object*>(url)->isa == &kURLClass ? static_cast<const URL*>(url)->path : std::string();
}
}
}
