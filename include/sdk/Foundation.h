#pragma once

class NSAutoreleasePool {
public:
    static NSAutoreleasePool* alloc();
    NSAutoreleasePool* init();
    void release();
};

class NSError;
class NSString;
class NSCharacterSet;
class NSNumber {
public:
    unsigned long long unsignedLongLongValue();
};
class NSDictionary {
public:
    void* objectForKey(NSString* key);
};
class NSFileManager {
public:
    static NSFileManager* defaultManager();
    NSDictionary* attributesOfFileSystemForPath_error(NSString* path, NSError** error);
    unsigned char fileExistsAtPath_(NSString* path);
};
extern NSString* const NSFileSystemFreeSize;

typedef struct objc_selector* SEL;
extern "C" SEL sel_registerName(const char* name);

class NSObject {
public:
    void release();
    void retain();
    void performSelectorOnMainThread_withObject_waitUntilDone(SEL selector, void* arg, bool wait);
};

typedef int NSComparisonResult;
enum { NSOrderedAscending = -1, NSOrderedSame, NSOrderedDescending };
enum { NSNumericSearch = 64 };

class NSString : public NSObject {
public:
    NSComparisonResult compare_options(NSString* other, unsigned int options);
    NSString* stringByTrimmingCharactersInSet(NSCharacterSet* set);
    bool hasPrefix(NSString* prefix);
    bool hasSuffix(NSString* suffix);
    NSString* substringFromIndex(int index);
    static unsigned defaultCStringEncoding();
    const char* cStringUsingEncoding(unsigned encoding);
    int compare(NSString* other);
};

class NSCharacterSet : public NSObject {
public:
    static NSCharacterSet* whitespaceCharacterSet();
};

extern "C" void* NSClassFromString(NSString* name);

class NSRecursiveLock : public NSObject {
public:
    static NSRecursiveLock* alloc();
    NSRecursiveLock* init();
    void lock();
    void unlock();
};

class NSLock : public NSObject {
public:
    static NSLock* alloc();
    NSLock* init();
    bool tryLock();
    void unlock();
};

class NSBundle : public NSObject {
public:
    static NSBundle* mainBundle();
    NSString* pathForResource_ofType_inDirectory(NSString* name, NSString* ext, NSString* directory);
};

class NSURL : public NSObject {
public:
    static NSURL* URLWithString(NSString* string);
    static NSURL* fileURLWithPath(NSString* path);
};

class NSData : public NSObject {
public:
    static NSData* dataWithContentsOfURL(NSURL* url);
};
