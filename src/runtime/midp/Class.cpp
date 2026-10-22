#include "runtime/midp/Class.h"

#include <stddef.h>

#include "runtime/midp/File.h"
#include "runtime/midp/FileInputStreamIPhone.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

void* operator new(size_t);
#include <new>

namespace midp {

Class* Class::getClass() const {
    return const_cast<Class*>(this);
}

InputStream* Class::getResourceAsStream(String* name) {
    ReferenceCountedHardReference hold(name);
    ReferenceCountedPointer<String> path(NULL);
    ReferenceCountedPointer<String> documents(File::getDocumentsFolder());
    ReferenceCountedPointer<File> file(new File(documents, name));
    if (file->exists())
        path = file->getPath();
    else
        path = File::getResFilename(name);
    if (path == NULL)
        path = name;
    FileInputStreamIPhone* stream = new FileInputStreamIPhone(path);
    if (stream->loadSuccessful())
        return stream;
    delete stream;
    return NULL;
}

Class::~Class() {}

Class::Class() {}

}
