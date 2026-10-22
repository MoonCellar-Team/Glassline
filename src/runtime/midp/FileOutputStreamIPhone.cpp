#include "runtime/midp/FileOutputStreamIPhone.h"

#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

namespace midp {

bool FileOutputStreamIPhone::loadSuccessful() {
    return m_file != NULL;
}

bool FileOutputStreamIPhone::seek(long offset, int whence) {
    if ((unsigned)whence > 2)
        return false;
    return fseek(m_file, offset, whence) == 0;
}

long FileOutputStreamIPhone::getPosition() {
    return ftell(m_file);
}

bool FileOutputStreamIPhone::close() {
    if (m_file != NULL) {
        fclose(m_file);
        m_file = NULL;
    }
    return true;
}

void FileOutputStreamIPhone::write(const void* data, int length) {
    if (m_file == NULL)
        return;
    fwrite(data, (size_t)length, 1, m_file);
}

void FileOutputStreamIPhone::write(unsigned char b) {
    if (m_file == NULL)
        return;
    fwrite(&b, 1, 1, m_file);
}

FileOutputStreamIPhone::~FileOutputStreamIPhone() {
    close();
}

FileOutputStreamIPhone::FileOutputStreamIPhone(String* name) : m_file(NULL) {
    ReferenceCountedHardReference hold(name);
    char path[512];
    CFStringGetCString(name->getCFString(), path, 512, kCFStringEncodingUTF8);
    m_file = fopen(path, "wb");
}

}
