#include "runtime/midp/FileInputStreamIPhone.h"

#include <stdio.h>
#include <sys/stat.h>

#include "port/files.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"

extern "C" bool CFStringGetCString(const void* string, char* buffer, long size, unsigned encoding);

namespace midp {

bool FileInputStreamIPhone::loadSuccessful() { return m_file != NULL; }

bool FileInputStreamIPhone::markSupported() const { return m_file != NULL; }

int FileInputStreamIPhone::size() { return m_size; }

bool FileInputStreamIPhone::seek(long offset, int whence) {
    long pos;
    if (whence == 0)
        pos = offset;
    else if (whence == 2)
        pos = (long)m_size + offset;
    else if (whence == 1)
        pos = (long)getPosition() + offset;
    else
        return false;
    bool inRange = pos >= 0;
    if (pos < 0)
        pos = 0;
    if (m_size < pos) {
        pos = m_size;
        inRange = false;
    }
    if (m_size > pos)
        m_eof = false;
    return fseek(m_file, pos, SEEK_SET) != 0 && inRange;
}

int FileInputStreamIPhone::getPosition() { return (int)ftell(m_file); }

void FileInputStreamIPhone::mark(int) {
    if (m_file != NULL)
        m_mark = ftell(m_file);
}

int FileInputStreamIPhone::available() const { return m_size - (int)ftell(m_file); }

void FileInputStreamIPhone::reset() {
    if (m_file == NULL)
        return;
    fseek(m_file, m_mark, SEEK_SET);
    m_eof = feof(m_file) != 0;
}

int FileInputStreamIPhone::read(signed char* buf, int n) {
    if (m_eof)
        return -1;
    int nread = (int)fread(buf, 1, (size_t)n, m_file);
    if (nread < n)
        m_eof = true;
    if (nread == 0)
        return -1;
    return nread;
}

int FileInputStreamIPhone::read() {
    if (m_eof)
        return -1;
    int c = getc_unlocked(m_file);
    if (c == EOF) {
        m_eof = true;
        return -1;
    }
    return c;
}

void FileInputStreamIPhone::close() {
    InputStream::close();
    if (m_file == NULL)
        return;
    funlockfile(m_file);
    fclose(m_file);
    m_file = NULL;
}

FileInputStreamIPhone::~FileInputStreamIPhone() { close(); }

FileInputStreamIPhone::FileInputStreamIPhone(String* path) : m_file(NULL), m_mark(0), m_size(0), m_eof(false) {
    ReferenceCountedHardReference hold(path);
    char name[512];
    CFStringGetCString(path->getCFString(), name, 512, 0x08000100);
    m_file = port::files::open(name);
    if (m_file == NULL) {
        m_eof = true;
        return;
    }
    m_mark = 0;
    flockfile(m_file);
    struct stat st;
    m_size = fstat(fileno(m_file), &st) == 0 ? (int)st.st_size : 0;
}

}
