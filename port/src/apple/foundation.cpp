#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <mutex>
#include <string>
#include <vector>

#include "port/cf.h"
#include "port/files.h"

using port::cf::Object;

namespace port {
namespace cf {
const void* makeDictionary(const std::vector<std::pair<const void*, const void*>>& items);
const void* dictionaryValue(const void* dict, const void* key);
const void* makeNumber(long long v);
long long numberValue(const void* n);
const void* makeFileURL(const std::string& path);
std::string urlPath(const void* url);
}
}

extern "C" {
const void* CFBundleGetMainBundle(void);
const void* CFBundleGetValueForInfoDictionaryKey(const void* bundle, const void* key);
const void* CFBundleCopyResourceURL(const void* bundle, const void* name, const void* type, const void* subdir);
long CFStringCompare(const void* a, const void* b, unsigned options);
extern int __CFConstantStringClassReference[32];
}

#define NS(sym) __asm__(sym)

namespace {

struct Pool {
    std::vector<const void*> objects;
};
thread_local std::vector<Pool*> t_pools;

const void* autorelease(const void* o) {
    if (o) {
        if (t_pools.empty())
            t_pools.push_back(new Pool());
        t_pools.back()->objects.push_back(o);
    }
    return o;
}

struct CString : Object {
    std::string s;
    explicit CString(const std::string& v);
};
void destroyCString(Object* o) { delete static_cast<CString*>(o); }
const port::cf::Class kCStringClass = {"NSCString", destroyCString, nullptr};
CString::CString(const std::string& v) : Object(&kCStringClass), s(v) {}

struct Lock : Object {
    std::mutex m;
    Lock();
};
void destroyLock(Object* o) { delete static_cast<Lock*>(o); }
const port::cf::Class kLockClass = {"NSLock", destroyLock, nullptr};
Lock::Lock() : Object(&kLockClass) {}

struct RecursiveLock : Object {
    std::recursive_mutex m;
    RecursiveLock();
};
void destroyRecursiveLock(Object* o) { delete static_cast<RecursiveLock*>(o); }
const port::cf::Class kRecursiveLockClass = {"NSRecursiveLock", destroyRecursiveLock, nullptr};
RecursiveLock::RecursiveLock() : Object(&kRecursiveLockClass) {}

struct Data : Object {
    std::vector<unsigned char> bytes;
    Data();
};
void destroyData(Object* o) { delete static_cast<Data*>(o); }
const port::cf::Class kDataClass = {"NSData", destroyData, nullptr};
Data::Data() : Object(&kDataClass) {}

void neverDestroy(Object*) {}
const port::cf::Class kSingletonClass = {"NSObject", neverDestroy, nullptr};
Object s_fileManager(&kSingletonClass);
Object s_whitespace(&kSingletonClass);

bool isWhitespace(char16_t c) {
    return c == u' ' || c == u'\t' || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x202F ||
           c == 0x205F || c == 0x3000;
}

struct ConstString {
    const void* isa;
    int32_t flags;
    const char* str;
    long length;
};
const ConstString s_freeSize = {__CFConstantStringClassReference, 0x07C8, "NSFileSystemFreeSize", 20};

}

extern "C" {

extern const void* const NSFileSystemFreeSize = &s_freeSize;

void* NSAutoreleasePool_alloc() NS("_ZN17NSAutoreleasePool5allocEv");
void* NSAutoreleasePool_alloc() { return new Pool(); }
void* NSAutoreleasePool_init(void* self) NS("_ZN17NSAutoreleasePool4initEv");
void* NSAutoreleasePool_init(void* self) {
    t_pools.push_back(static_cast<Pool*>(self));
    return self;
}
void NSAutoreleasePool_release(void* self) NS("_ZN17NSAutoreleasePool7releaseEv");
void NSAutoreleasePool_release(void* self) {
    Pool* pool = static_cast<Pool*>(self);
    while (!t_pools.empty()) {
        Pool* top = t_pools.back();
        t_pools.pop_back();
        for (const void* o : top->objects) CFRelease(o);
        if (top == pool) break;
        delete top;
    }
    delete pool;
}

void NSObject_retain(void* self) NS("_ZN8NSObject6retainEv");
void NSObject_retain(void* self) { CFRetain(self); }
void NSObject_release(void* self) NS("_ZN8NSObject7releaseEv");
void NSObject_release(void* self) { CFRelease(self); }

void* NSString_substringFromIndex(void* self, int i) NS("_ZN8NSString18substringFromIndexEi");
void* NSString_substringFromIndex(void* self, int i) {
    std::u16string t = port::cf::text(self);
    return const_cast<void*>(autorelease(port::cf::makeString(i < static_cast<int>(t.size()) ? t.substr(i) : std::u16string())));
}
unsigned NSString_defaultCStringEncoding() NS("_ZN8NSString22defaultCStringEncodingEv");
unsigned NSString_defaultCStringEncoding() { return 4; }
const char* NSString_cStringUsingEncoding(void* self, unsigned) NS("_ZN8NSString20cStringUsingEncodingEj");
const char* NSString_cStringUsingEncoding(void* self, unsigned) {
    CString* c = new CString(port::cf::utf8(self));
    autorelease(c);
    return c->s.c_str();
}
void* NSString_stringByTrimmingCharactersInSet(void* self, void* set) NS("_ZN8NSString31stringByTrimmingCharactersInSetEP14NSCharacterSet");
void* NSString_stringByTrimmingCharactersInSet(void* self, void* set) {
    std::u16string t = port::cf::text(self);
    if (set == &s_whitespace) {
        size_t a = 0, b = t.size();
        while (a < b && isWhitespace(t[a])) a++;
        while (b > a && isWhitespace(t[b - 1])) b--;
        t = t.substr(a, b - a);
    }
    return const_cast<void*>(autorelease(port::cf::makeString(t)));
}
int NSString_compare(void* self, void* other) NS("_ZN8NSString7compareEPS_");
int NSString_compare(void* self, void* other) { return static_cast<int>(CFStringCompare(self, other, 0)); }
bool NSString_hasPrefix(void* self, void* prefix) NS("_ZN8NSString9hasPrefixEPS_");
bool NSString_hasPrefix(void* self, void* prefix) {
    std::u16string t = port::cf::text(self), p = port::cf::text(prefix);
    return !p.empty() && t.compare(0, p.size(), p) == 0;
}
bool NSString_hasSuffix(void* self, void* suffix) NS("_ZN8NSString9hasSuffixEPS_");
bool NSString_hasSuffix(void* self, void* suffix) {
    std::u16string t = port::cf::text(self), s = port::cf::text(suffix);
    return !s.empty() && t.size() >= s.size() && t.compare(t.size() - s.size(), s.size(), s) == 0;
}

void* NSCharacterSet_whitespaceCharacterSet() NS("_ZN14NSCharacterSet22whitespaceCharacterSetEv");
void* NSCharacterSet_whitespaceCharacterSet() { return &s_whitespace; }

void* NSLock_alloc() NS("_ZN6NSLock5allocEv");
void* NSLock_alloc() { return new Lock(); }
void* NSLock_init(void* self) NS("_ZN6NSLock4initEv");
void* NSLock_init(void* self) { return self; }
bool NSLock_tryLock(void* self) NS("_ZN6NSLock7tryLockEv");
bool NSLock_tryLock(void* self) { return static_cast<Lock*>(self)->m.try_lock(); }
void NSLock_unlock(void* self) NS("_ZN6NSLock6unlockEv");
void NSLock_unlock(void* self) { static_cast<Lock*>(self)->m.unlock(); }

void* NSRecursiveLock_alloc() NS("_ZN15NSRecursiveLock5allocEv");
void* NSRecursiveLock_alloc() { return new RecursiveLock(); }
void* NSRecursiveLock_init(void* self) NS("_ZN15NSRecursiveLock4initEv");
void* NSRecursiveLock_init(void* self) { return self; }
void NSRecursiveLock_lock(void* self) NS("_ZN15NSRecursiveLock4lockEv");
void NSRecursiveLock_lock(void* self) { static_cast<RecursiveLock*>(self)->m.lock(); }
void NSRecursiveLock_unlock(void* self) NS("_ZN15NSRecursiveLock6unlockEv");
void NSRecursiveLock_unlock(void* self) { static_cast<RecursiveLock*>(self)->m.unlock(); }

void* NSBundle_mainBundle() NS("_ZN8NSBundle10mainBundleEv");
void* NSBundle_mainBundle() { return const_cast<void*>(CFBundleGetMainBundle()); }
const void* NSBundle_objectForInfoDictionaryKey(void* self, const void* key) NS("_ZN8NSBundle26objectForInfoDictionaryKeyEPK10__CFString");
const void* NSBundle_objectForInfoDictionaryKey(void* self, const void* key) {
    return CFBundleGetValueForInfoDictionaryKey(self, key);
}
void* NSBundle_pathForResource(void* self, void* name, void* ext, void* dir) NS("_ZN8NSBundle34pathForResource_ofType_inDirectoryEP8NSStringS1_S1_");
void* NSBundle_pathForResource(void* self, void* name, void* ext, void* dir) {
    const void* url = CFBundleCopyResourceURL(self, name, ext, dir);
    if (!url) return nullptr;
    const void* path = port::cf::makeString(port::cf::urlPath(url));
    CFRelease(url);
    return const_cast<void*>(autorelease(path));
}

void* NSURL_fileURLWithPath(void* path) NS("_ZN5NSURL15fileURLWithPathEP8NSString");
void* NSURL_fileURLWithPath(void* path) {
    return const_cast<void*>(autorelease(port::cf::makeFileURL(port::cf::utf8(path))));
}
void* NSURL_URLWithString(void* s) NS("_ZN5NSURL13URLWithStringEP8NSString");
void* NSURL_URLWithString(void* s) {
    std::string u = port::cf::utf8(s);
    if (u.compare(0, 7, "file://") == 0) u = u.substr(7);
    return const_cast<void*>(autorelease(port::cf::makeFileURL(u)));
}
void* NSData_dataWithContentsOfURL(void* url) NS("_ZN6NSData21dataWithContentsOfURLEP5NSURL");
void* NSData_dataWithContentsOfURL(void* url) {
    Data* d = new Data();
    if (!port::files::read(port::cf::urlPath(url), &d->bytes)) {
        CFRelease(d);
        return nullptr;
    }
    return const_cast<void*>(autorelease(d));
}

void* NSFileManager_defaultManager() NS("_ZN13NSFileManager14defaultManagerEv");
void* NSFileManager_defaultManager() { return &s_fileManager; }
unsigned char NSFileManager_fileExistsAtPath(void* self, void* path) NS("_ZN13NSFileManager17fileExistsAtPath_EP8NSString");
unsigned char NSFileManager_fileExistsAtPath(void* self, void* path) {
    return port::files::stat(port::cf::utf8(path)).exists;
}
void* NSFileManager_attributesOfFileSystem(void* self, void* path, void** error) NS("_ZN13NSFileManager35attributesOfFileSystemForPath_errorEP8NSStringPP7NSError");
void* NSFileManager_attributesOfFileSystem(void* self, void* path, void** error) {
    struct statvfs fs;
    if (statvfs(port::cf::utf8(path).c_str(), &fs) != 0) {
        if (error) *error = nullptr;
        return nullptr;
    }
    long long free = static_cast<long long>(fs.f_bavail) * fs.f_frsize;
    const void* key = CFRetain(NSFileSystemFreeSize);
    return const_cast<void*>(autorelease(port::cf::makeDictionary({{key, port::cf::makeNumber(free)}})));
}
void* NSDictionary_objectForKey(void* self, void* key) NS("_ZN12NSDictionary12objectForKeyEP8NSString");
void* NSDictionary_objectForKey(void* self, void* key) {
    return const_cast<void*>(port::cf::dictionaryValue(self, key));
}
unsigned long long NSNumber_unsignedLongLongValue(void* self) NS("_ZN8NSNumber21unsignedLongLongValueEv");
unsigned long long NSNumber_unsignedLongLongValue(void* self) {
    return static_cast<unsigned long long>(port::cf::numberValue(self));
}

}
