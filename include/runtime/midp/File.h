#pragma once

#include "runtime/midp/Object.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {

class Class;
class String;

class File : public Object {
public:
    explicit File(String* path);
    File(String* parent, String* name);
    virtual ~File();
    virtual Class* getClass() const;
    using Object::equals;
    bool equals(Object* other);
    String* toString();
    bool exists();
    String* getPath();
    String* getParent();
    String* getName();
    static String* getDocumentsFolder();
    static String* getResFilename(String* name);
    static String* getBundleFilename(String* name);
    static String* getLocalizedFilename(String* name, String* locale);
    static String* getDocumentsFilename(String* name);

    static unsigned short pathSeparatorChar;
    ReferenceCountedPointer<String> m_path;
};

}
