#include "game/Util.h"

#include "runtime/midp/File.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "sdk/Foundation.h"

unsigned long long Util::getFreeFSBytes() {
    midp::ReferenceCountedPointer<midp::String> folder(midp::File::getDocumentsFolder());
    NSString* path = (NSString*)folder->getCFString();
    NSDictionary* attributes = NSFileManager::defaultManager()->attributesOfFileSystemForPath_error(path, NULL);
    NSNumber* freeSize = (NSNumber*)attributes->objectForKey(NSFileSystemFreeSize);
    return freeSize->unsignedLongLongValue();
}
