#pragma once

#include "m3g/OpaqueObject.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"

namespace midp {
class InputStream;
class String;
}

class LocaleManager {
public:
    typedef midp::array<midp::ReferenceCountedPointer<midp::String>> StringList;

    LocaleManager();
    virtual ~LocaleManager();

    static LocaleManager* getInstance();
    void setStringIdBits(int bits);
    midp::String* getLocale() const;
    midp::InputStream* getLocalizedResourceAsStream(midp::String* name, midp::String* locale) const;
    void extendPools(int count);
    StringList getSupportedLocales() const;
    StringList loadStrings(midp::String* name, midp::String* locale);
    int getLocaleIndex(midp::String* locale) const;
    midp::String* getString(int id, midp::String* locale) const;
    void setStringPool(int pool, StringList strings, midp::String* locale);
    void setLocale(midp::String* locale);
    midp::String* getSupportedLang(midp::String* locale) const;

    static LocaleManager s_instance;
    static const unsigned char FILE_IDENTIFIER[13];

    m3g::OpaqueObject<util::VectorSE<midp::String>, 0x18> m_locales;
    midp::String* m_locale;
    int m_localeIndex;
    int m_poolMask;
    int m_stringIdBits;
    int m_stringIdMask;
    midp::array<midp::array<StringList>> m_pools;
};
