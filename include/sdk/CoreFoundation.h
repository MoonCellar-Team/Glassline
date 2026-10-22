#pragma once

extern "C" {

struct __CFString;
typedef const struct __CFString* CFStringRef;

int CFStringGetIntValue(CFStringRef string);

typedef const void* CFTypeRef;
typedef long CFIndex;
typedef const struct __CFArray* CFArrayRef;
typedef struct __CFBundle* CFBundleRef;

CFTypeRef CFRetain(CFTypeRef cf);
void CFRelease(CFTypeRef cf);
CFIndex CFArrayGetCount(CFArrayRef array);
const void* CFArrayGetValueAtIndex(CFArrayRef array, CFIndex index);
CFBundleRef CFBundleGetMainBundle(void);
CFArrayRef CFBundleCopyBundleLocalizations(CFBundleRef bundle);
#define CFSTR(s) ((CFStringRef)__builtin___CFStringMakeConstantString(s))

typedef unsigned int CFStringEncoding;
enum { kCFStringEncodingUTF8 = 0x08000100 };
int CFStringGetCString(CFStringRef string, char* buffer, CFIndex bufferSize, CFStringEncoding encoding);
CFStringRef CFBundleGetValueForInfoDictionaryKey(CFBundleRef bundle, CFStringRef key);
extern CFStringRef kCFBundleVersionKey;
CFStringEncoding CFStringGetSystemEncoding(void);
const char* CFStringGetCStringPtr(CFStringRef string, CFStringEncoding encoding);

typedef const struct __CFAllocator* CFAllocatorRef;
extern const CFAllocatorRef kCFAllocatorDefault;
typedef struct __CFString* CFMutableStringRef;
typedef struct {
    CFIndex location;
    CFIndex length;
} CFRange;

typedef struct __CFURL* CFURLRef;
CFURLRef CFBundleCopyBundleURL(CFBundleRef bundle);
CFURLRef CFBundleCopyResourceURL(CFBundleRef bundle, CFStringRef resourceName, CFStringRef resourceType,
                                 CFStringRef subDirName);
CFURLRef CFURLCreateCopyDeletingLastPathComponent(CFAllocatorRef allocator, CFURLRef url);
CFURLRef CFURLCreateCopyAppendingPathComponent(CFAllocatorRef allocator, CFURLRef url, CFStringRef pathComponent,
                                              bool isDirectory);
bool CFURLGetFileSystemRepresentation(CFURLRef url, bool resolveAgainstBase, char* buffer, CFIndex maxBufLen);

CFMutableStringRef CFStringCreateMutable(CFAllocatorRef alloc, CFIndex maxLength);
void CFStringAppend(CFMutableStringRef string, CFStringRef appended);
void CFStringAppendCString(CFMutableStringRef string, const char* cstr, CFStringEncoding encoding);
void CFStringAppendFormat(CFMutableStringRef string, void* formatOptions, CFStringRef format, ...);
CFIndex CFStringGetLength(CFStringRef string);
void CFStringDelete(CFMutableStringRef string, CFRange range);
enum { kCFStringEncodingASCII = 0x600 };

}
