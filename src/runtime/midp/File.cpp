#include "runtime/midp/File.h"

#include "runtime/midp/FileClass.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#include "sdk/CoreFoundation.h"
#include "sdk/Foundation.h"

void* operator new(size_t);

namespace midp {

unsigned short File::pathSeparatorChar = '/';

namespace {

String* joinRelative(String* parent, String* middle, String* tail) {
    StringBuffer buf;
    buf.setLength(0);
    if (parent != NULL) {
        buf.append(parent);
        buf.append(File::pathSeparatorChar);
    }
    if (middle != NULL) {
        buf.append(middle);
        buf.append(File::pathSeparatorChar);
    }
    if (tail != NULL)
        buf.append(tail);
    return buf.toString();
}

}

Class* File::getClass() const {
    return new FileClass();
}

bool File::equals(Object*) {
    return false;
}

String* File::toString() {
    return NULL;
}

String* File::getPath() {
    return m_path;
}

File::~File() {}

File::File(String* path) : m_path(path) {}

String* File::getParent() {
    const int slash = m_path->lastIndexOf(pathSeparatorChar);
    if (slash == -1)
        return NULL;
    if (slash < 2)
        return new String("/");
    return m_path->substring(0, slash);
}

String* File::getName() {
    const int slash = m_path->lastIndexOf(pathSeparatorChar);
    if (slash >= 0)
        return m_path->substring(slash + 1, m_path->length());
    return m_path;
}

bool File::exists() {
    if (m_path == NULL)
        return false;
    if (m_path->getCFString() == NULL)
        return false;
    if (m_path->length() <= 0)
        return false;
    NSFileManager* manager = NSFileManager::defaultManager();
    const unsigned char found =
        manager->fileExistsAtPath_(reinterpret_cast<NSString*>(const_cast<__CFString*>(m_path->getCFString())));
    return (found & 0xff) == 1;
}

File::File(String* parent, String* name) : m_path(NULL) {
    ReferenceCountedPointer<String> parentHold(parent);
    ReferenceCountedPointer<String> nameHold(name);
    StringBuffer buf;
    buf.append(static_cast<String*>(parentHold));
    if (parentHold->charAt(parentHold->length() - 1) != pathSeparatorChar)
        buf.append(pathSeparatorChar);
    buf.append(static_cast<String*>(nameHold));
    m_path = buf.toString();
}

String* File::getBundleFilename(String* path) {
    File file(path);
    ReferenceCountedPointer<String> name(file.getName());
    ReferenceCountedPointer<String> parent(file.getParent());
    CFStringRef subdirectory = NULL;
    if (parent != NULL)
        subdirectory = parent->getCFString();
    CFURLRef url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), name->getCFString(), NULL, subdirectory);
    String* result = NULL;
    if (url != NULL) {
        char buffer[0x100];
        CFURLGetFileSystemRepresentation(url, true, buffer, 0x100);
        CFRelease(url);
        result = new String(buffer);
    }
    return result;
}

String* File::getResFilename(String* name) {
    String* res = new String("res");
    File file(res, name);
    ReferenceCountedPointer<String> fileName(file.getName());
    ReferenceCountedPointer<String> parent(file.getParent());
    CFURLRef url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), fileName->getCFString(), NULL,
                                           parent->getCFString());
    String* result = NULL;
    if (url != NULL) {
        char buffer[0x100];
        CFURLGetFileSystemRepresentation(url, true, buffer, 0x100);
        CFRelease(url);
        result = new String(buffer);
    }
    return result;
}

String* File::getDocumentsFolder() {
    CFURLRef bundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef parent = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, bundleURL);
    CFURLRef documents =
        CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, parent, CFSTR("Documents"), true);
    char buffer[0x100];
    CFURLGetFileSystemRepresentation(documents, true, buffer, 0x100);
    CFRelease(bundleURL);
    CFRelease(parent);
    CFRelease(documents);
    return new String(buffer);
}

String* File::getLocalizedFilename(String* name, String* locale) {
    ReferenceCountedHardReference hold(name);
    File base(name);
    ReferenceCountedPointer<String> parent(base.getParent());
    ReferenceCountedPointer<String> fileName(base.getName());
    ReferenceCountedPointer<String> lproj(NULL);
    if (locale != NULL) {
        StringBuffer localeDir;
        localeDir.append(locale);
        localeDir.append(".lproj");
        lproj = localeDir.toString();
    }
    ReferenceCountedPointer<String> documents(getDocumentsFolder());
    String* result = NULL;
    if (lproj != NULL) {
        StringBuffer path;
        path.append(static_cast<String*>(documents));
        path.append(pathSeparatorChar);
        if (parent != NULL) {
            path.append(static_cast<String*>(parent));
            path.append(pathSeparatorChar);
        }
        path.append(static_cast<String*>(lproj));
        path.append(pathSeparatorChar);
        File candidate(path.toString(), fileName);
        const bool found = candidate.exists();
        if (found)
            result = new String(candidate.getPath());
        if (found)
            return result;
    }
    File inDocuments(static_cast<String*>(documents), name);
    if (inDocuments.exists())
        return new String(inDocuments.getPath());
    if (lproj != NULL) {
        String* located = getResFilename(joinRelative(parent, lproj, fileName));
        if (located != NULL)
            return located;
    }
    String* located = getResFilename(joinRelative(parent, NULL, fileName));
    if (located != NULL)
        return located;
    return getBundleFilename(joinRelative(parent, NULL, fileName));
}

String* File::getDocumentsFilename(String* name) {
    ReferenceCountedPointer<String> folder(getDocumentsFolder());
    File* file = new File(static_cast<String*>(folder), name);
    INCREF(file);
    String* path = file->getPath();
    INCREF(path);
    DECREF(file);
    DECREF_NO_DELETE(path);
    return path;
}

}
