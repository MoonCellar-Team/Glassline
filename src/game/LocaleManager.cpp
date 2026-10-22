#include "game/LocaleManager.h"

#include <stddef.h>

#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/File.h"
#include "runtime/midp/FileInputStreamIPhone.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "runtime/midp/System.h"
#include "sdk/CoreFoundation.h"

static const int DEFAULT_STRING_ID_BITS = 16;
static const int LANGUAGE_CODE_LENGTH = 2;

const unsigned char LocaleManager::FILE_IDENTIFIER[13] = {0xab, 'S', 't', 'r', 'i', 'n', 'g',
                                                          's',  0xbb, 0x0d, 0x0a, 0x1a, 0x0a};

LocaleManager* LocaleManager::getInstance() {
    return &s_instance;
}

void LocaleManager::setStringIdBits(int bits) {
    m_poolMask = (int)(0x80000000u - (1u << bits));
    m_stringIdBits = bits;
    m_stringIdMask = (1 << bits) - 1;
}

midp::String* LocaleManager::getLocale() const {
    return m_locale;
}

midp::InputStream* LocaleManager::getLocalizedResourceAsStream(midp::String* name, midp::String* locale) const {
    if (locale == NULL) {
        locale = getLocale();
    }
    midp::ReferenceCountedPointer<midp::String> localeRef(locale);
    midp::ReferenceCountedPointer<midp::String> path(midp::File::getLocalizedFilename(name, localeRef));
    if (path == NULL) {
        return NULL;
    }
    midp::FileInputStreamIPhone* stream = new midp::FileInputStreamIPhone(path);
    if (!stream->loadSuccessful()) {
        delete stream;
        return NULL;
    }
    return stream;
}

void LocaleManager::extendPools(int count) {
    midp::array<midp::array<StringList>> pools(count);
    int n = m_pools.m_length < count ? count : m_pools.m_length;
    for (int i = 0; i < n; i++) {
        if (i < m_pools.m_length) {
            pools[i] = m_pools[i];
        } else {
            pools[i] = midp::array<StringList>(m_locales.get()->size());
        }
    }
    m_pools = pools;
}

LocaleManager::StringList LocaleManager::getSupportedLocales() const {
    StringList locales(m_locales.get()->size());
    for (int i = 0; i < locales.m_length; i++) {
        locales[i] = m_locales.get()->elementAt(i);
    }
    return locales;
}

LocaleManager::StringList LocaleManager::loadStrings(midp::String* name, midp::String* locale) {
    midp::DataInputStream in(getLocalizedResourceAsStream(name, locale));
    midp::array<unsigned char> header(sizeof(FILE_IDENTIFIER));
    in.readFully(header);
    int headerLength = header.m_length;
    for (int i = 0; i < headerLength; i++) {
        if (header[i] != FILE_IDENTIFIER[i]) {
            return StringList();
        }
    }
    midp::ReferenceCountedPointer<midp::String> language(in.readUTF());
    int count = in.readInt();
    StringList strings(count);
    for (int i = 0; i < count; i++) {
        strings[i] = in.readUTF();
    }
    return strings;
}

int LocaleManager::getLocaleIndex(midp::String* locale) const {
    if (locale == NULL) {
        return m_localeIndex;
    }
    midp::ReferenceCountedHardReference hold(locale);
    int count = m_locales.get()->size();
    for (int i = 0; i < count; i++) {
        if (m_locales.get()->elementAt(i)->equals(locale)) {
            return i;
        }
    }
    return -1;
}

midp::String* LocaleManager::getString(int id, midp::String* locale) const {
    int localeIndex = getLocaleIndex(locale);
    StringList pool = m_pools[(id & m_poolMask) >> m_stringIdBits][localeIndex];
    return pool[id & m_stringIdMask];
}

void LocaleManager::setStringPool(int pool, StringList strings, midp::String* locale) {
    int localeIndex = getLocaleIndex(locale);
    if (pool >= m_pools.m_length) {
        extendPools(pool + 1);
    }
    m_pools[pool][localeIndex] = strings;
}

void LocaleManager::setLocale(midp::String* locale) {
    midp::ReferenceCountedHardReference hold(locale);
    int index = getLocaleIndex(locale);
    if (index != -1) {
        m_localeIndex = index;
        midp::INCREF(locale);
        midp::DECREF(m_locale);
        m_locale = locale;
    }
}

midp::String* LocaleManager::getSupportedLang(midp::String* locale) const {
    midp::ReferenceCountedHardReference hold(locale);
    for (int i = 0; i < m_locales.get()->size(); i++) {
        midp::ReferenceCountedPointer<midp::String> lang(m_locales.get()->elementAt(i));
        if (lang->length() > LANGUAGE_CODE_LENGTH) {
            lang = lang->substring(0, LANGUAGE_CODE_LENGTH);
        }
        if (locale->startsWith(lang)) {
            return m_locales.get()->elementAt(i);
        }
    }
    return NULL;
}

LocaleManager::~LocaleManager() {}

LocaleManager LocaleManager::s_instance;

LocaleManager::LocaleManager()
    : m_locales(),
      m_locale(NULL),
      m_localeIndex(-1),
      m_poolMask(0),
      m_stringIdBits(0),
      m_stringIdMask(0),
      m_pools(0) {
    CFArrayRef localizations = CFBundleCopyBundleLocalizations(CFBundleGetMainBundle());
    int count = CFArrayGetCount(localizations);
    for (int i = 0; i < count; i++) {
        CFStringRef name = static_cast<CFStringRef>(CFArrayGetValueAtIndex(localizations, i));
        CFRetain(name);
        midp::ReferenceCountedPointer<midp::String> locale(new midp::String(name));
        bool unique = true;
        for (int j = 0; j < m_locales.get()->size() && unique; j++) {
            unique = !m_locales.get()->elementAt(j)->equals(locale);
        }
        if (unique) {
            m_locales.get()->addElement(locale);
        }
    }
    CFRelease(localizations);
    setStringIdBits(DEFAULT_STRING_ID_BITS);
    midp::ReferenceCountedPointer<midp::String> systemLocale(
        midp::System::getProperty(new midp::String("microedition.locale")));
    midp::ReferenceCountedPointer<midp::String> locale(getSupportedLang(systemLocale));
    if (!locale) {
        locale = m_locales.get()->elementAt(0);
    }
    setLocale(locale);
}
