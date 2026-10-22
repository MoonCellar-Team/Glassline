#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include <string>

#include "port/cf.h"

typedef long CFIndex;
typedef unsigned int CFStringEncoding;
struct CFRange {
    CFIndex location;
    CFIndex length;
};

namespace {

const CFIndex kCFNotFound = -1;

enum : CFStringEncoding {
    kMacRoman = 0,
    kWindowsLatin1 = 0x0500,
    kISOLatin1 = 0x0201,
    kNextStepLatin = 0x0B01,
    kASCII = 0x0600,
    kUnicode = 0x0100,
    kUTF8 = 0x08000100,
    kUTF16BE = 0x10000100,
    kUTF16LE = 0x14000100,
};

enum {
    kCompareCaseInsensitive = 1,
    kCompareBackwards = 4,
    kCompareAnchored = 8,
    kCompareNumerically = 64,
};

struct ConstString {
    const void* isa;
    int32_t flags;
    const void* str;
    long length;
};
const int32_t kConstUtf16 = 0x07D0;

struct String : port::cf::Object {
    std::u16string s;
    std::string cstr;
    bool cstrValid = false;
    explicit String(const std::u16string& v);
};

void destroyString(port::cf::Object* o) { delete static_cast<String*>(o); }
std::u16string describeString(const port::cf::Object* o) { return static_cast<const String*>(o)->s; }
const port::cf::Class kStringClass = {"CFString", destroyString, describeString};

String::String(const std::u16string& v) : port::cf::Object(&kStringClass), s(v) {}

bool isConst(const void* p);

String* mut(const void* p) {
    if (!p || isConst(p) || static_cast<const port::cf::Object*>(p)->isa != &kStringClass) {
        fprintf(stderr, "medecomp: изменение неизменяемой строки CF\n");
        abort();
    }
    String* s = static_cast<String*>(const_cast<void*>(p));
    s->cstrValid = false;
    return s;
}

bool singleByte(CFStringEncoding e) {
    return e == kMacRoman || e == kWindowsLatin1 || e == kISOLatin1 || e == kNextStepLatin || e == kASCII;
}

bool isUtf8(const unsigned char* b, size_t n) {
    bool wide = false;
    for (size_t i = 0; i < n;) {
        unsigned char c = b[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
        if (len == 0 || i + len > n) return false;
        for (size_t k = 1; k < len; k++)
            if ((b[i + k] & 0xC0) != 0x80) return false;
        wide = wide || len > 1;
        i += len;
    }
    return wide;
}

std::u16string decode(const unsigned char* b, size_t n, CFStringEncoding enc) {
    if (enc == kUTF8 || (enc == kASCII && isUtf8(b, n)))
        return port::cf::fromUtf8(reinterpret_cast<const char*>(b), n);
    if (enc == kUnicode || enc == kUTF16BE || enc == kUTF16LE) {
        bool be = enc == kUTF16BE;
        size_t i = 0;
        if (enc == kUnicode && n >= 2) {
            if (b[0] == 0xFE && b[1] == 0xFF) { be = true; i = 2; }
            else if (b[0] == 0xFF && b[1] == 0xFE) { i = 2; }
        }
        std::u16string out;
        for (; i + 1 < n; i += 2)
            out += static_cast<char16_t>(be ? (b[i] << 8 | b[i + 1]) : (b[i + 1] << 8 | b[i]));
        return out;
    }
    std::u16string out;
    for (size_t i = 0; i < n; i++)
        out += static_cast<char16_t>(b[i]);
    return out;
}

CFIndex encode(const std::u16string& s, CFIndex from, CFIndex to, CFStringEncoding enc, unsigned char lossByte,
               bool external, std::string& out) {
    CFIndex i = from;
    if (enc == kUTF8) {
        out = port::cf::toUtf8(s.substr(from, to - from));
        return to - from;
    }
    if (enc == kUnicode || enc == kUTF16BE || enc == kUTF16LE) {
        bool be = enc == kUTF16BE;
        if (enc == kUnicode && external) { out += '\xFF'; out += '\xFE'; }
        for (; i < to; i++) {
            char16_t c = s[i];
            if (be) { out += static_cast<char>(c >> 8); out += static_cast<char>(c & 0xFF); }
            else { out += static_cast<char>(c & 0xFF); out += static_cast<char>(c >> 8); }
        }
        return to - from;
    }
    unsigned limit = enc == kASCII ? 0x7F : 0xFF;
    for (; i < to; i++) {
        char16_t c = s[i];
        if (c <= limit) out += static_cast<char>(c);
        else if (lossByte) out += static_cast<char>(lossByte);
        else break;
    }
    return i - from;
}

char16_t fold(char16_t c) { return static_cast<char16_t>(towlower(c)); }

long compare(const std::u16string& a, const std::u16string& b, unsigned options) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        char16_t x = a[i], y = b[j];
        if ((options & kCompareNumerically) && x >= u'0' && x <= u'9' && y >= u'0' && y <= u'9') {
            size_t i2 = i, j2 = j;
            while (i2 < a.size() && a[i2] == u'0') i2++;
            while (j2 < b.size() && b[j2] == u'0') j2++;
            size_t ie = i2, je = j2;
            while (ie < a.size() && a[ie] >= u'0' && a[ie] <= u'9') ie++;
            while (je < b.size() && b[je] >= u'0' && b[je] <= u'9') je++;
            if (ie - i2 != je - j2) return ie - i2 < je - j2 ? -1 : 1;
            int c = a.compare(i2, ie - i2, b, j2, je - j2);
            if (c) return c < 0 ? -1 : 1;
            i = ie; j = je;
            continue;
        }
        if (options & kCompareCaseInsensitive) { x = fold(x); y = fold(y); }
        if (x != y) return x < y ? -1 : 1;
        i++; j++;
    }
    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
    return 0;
}

bool eqAt(const std::u16string& s, size_t pos, const std::u16string& what, bool ci) {
    if (pos + what.size() > s.size()) return false;
    for (size_t k = 0; k < what.size(); k++) {
        char16_t x = s[pos + k], y = what[k];
        if (ci) { x = fold(x); y = fold(y); }
        if (x != y) return false;
    }
    return true;
}

void appendAscii(std::u16string& out, const char* s) {
    while (*s) out += static_cast<char16_t>(static_cast<unsigned char>(*s++));
}

void formatInto(std::u16string& out, const std::u16string& f, va_list ap) {
    for (size_t i = 0; i < f.size();) {
        if (f[i] != u'%') { out += f[i++]; continue; }
        size_t start = i++;
        if (i < f.size() && f[i] == u'%') { out += u'%'; i++; continue; }
        size_t k = i;
        while (k < f.size() && f[k] >= u'0' && f[k] <= u'9') k++;
        if (k < f.size() && f[k] == u'$') i = k + 1;
        std::string spec = "%";
        while (i < f.size() && strchr("-+ #0'", static_cast<char>(f[i])) && f[i] < 0x80) spec += static_cast<char>(f[i++]);
        if (i < f.size() && f[i] == u'*') { spec += std::to_string(va_arg(ap, int)); i++; }
        while (i < f.size() && f[i] >= u'0' && f[i] <= u'9') spec += static_cast<char>(f[i++]);
        if (i < f.size() && f[i] == u'.') {
            spec += '.'; i++;
            if (i < f.size() && f[i] == u'*') { spec += std::to_string(va_arg(ap, int)); i++; }
            while (i < f.size() && f[i] >= u'0' && f[i] <= u'9') spec += static_cast<char>(f[i++]);
        }
        std::string len;
        while (i < f.size() && f[i] < 0x80 && strchr("hlqLztj", static_cast<char>(f[i]))) len += static_cast<char>(f[i++]);
        if (i >= f.size()) { out.append(f, start, std::u16string::npos); break; }
        char conv = f[i] < 0x80 ? static_cast<char>(f[i]) : '?';
        i++;
        char buf[512];
        if (len == "q") len = "ll";
        switch (conv) {
        case '@': out += port::cf::describe(va_arg(ap, const void*)); break;
        case 'C': out += static_cast<char16_t>(va_arg(ap, int)); break;
        case 'S': {
            const char16_t* p = va_arg(ap, const char16_t*);
            while (p && *p) out += *p++;
            break;
        }
        case 'c': out += static_cast<char16_t>(static_cast<unsigned char>(va_arg(ap, int))); break;
        case 's': {
            const char* p = va_arg(ap, const char*);
            snprintf(buf, sizeof buf, (spec + "s").c_str(), p ? p : "(null)");
            out += port::cf::fromUtf8(buf, strlen(buf));
            break;
        }
        case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': {
            std::string c = spec + len + conv;
            if (len == "ll" || len == "j") snprintf(buf, sizeof buf, c.c_str(), va_arg(ap, long long));
            else if (len == "l" || len == "z" || len == "t") snprintf(buf, sizeof buf, c.c_str(), va_arg(ap, long));
            else snprintf(buf, sizeof buf, c.c_str(), va_arg(ap, int));
            appendAscii(out, buf);
            break;
        }
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A': {
            if (len == "L") snprintf(buf, sizeof buf, (spec + "L" + conv).c_str(), va_arg(ap, long double));
            else snprintf(buf, sizeof buf, (spec + conv).c_str(), va_arg(ap, double));
            appendAscii(out, buf);
            break;
        }
        case 'p': snprintf(buf, sizeof buf, "%p", va_arg(ap, void*)); appendAscii(out, buf); break;
        default: out.append(f, start, i - start); break;
        }
    }
}

}

extern "C" int __CFConstantStringClassReference[32];
int __CFConstantStringClassReference[32];

namespace {
bool isConst(const void* p) { return p && static_cast<const ConstString*>(p)->isa == __CFConstantStringClassReference; }
}

namespace port {
namespace cf {

bool isString(const void* p) {
    return p && (isConst(p) || static_cast<const Object*>(p)->isa == &kStringClass);
}

std::u16string fromUtf8(const char* s, size_t n) {
    std::u16string out;
    const unsigned char* b = reinterpret_cast<const unsigned char*>(s);
    for (size_t i = 0; i < n;) {
        uint32_t c = b[i];
        int extra = c < 0x80 ? 0 : c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
        if (c >= 0x80 && c < 0xC0) { out += u'�'; i++; continue; }
        c &= extra == 0 ? 0x7F : extra == 1 ? 0x1F : extra == 2 ? 0x0F : 0x07;
        size_t j = 1;
        for (; j <= static_cast<size_t>(extra) && i + j < n && (b[i + j] & 0xC0) == 0x80; j++)
            c = c << 6 | (b[i + j] & 0x3F);
        i += j;
        if (c >= 0x10000) {
            c -= 0x10000;
            out += static_cast<char16_t>(0xD800 + (c >> 10));
            out += static_cast<char16_t>(0xDC00 + (c & 0x3FF));
        } else {
            out += static_cast<char16_t>(c);
        }
    }
    return out;
}

std::string toUtf8(const std::u16string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        uint32_t c = s[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] < 0xE000)
            c = 0x10000 + ((c - 0xD800) << 10) + (s[++i] - 0xDC00);
        if (c < 0x80) out += static_cast<char>(c);
        else if (c < 0x800) { out += static_cast<char>(0xC0 | c >> 6); out += static_cast<char>(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) {
            out += static_cast<char>(0xE0 | c >> 12); out += static_cast<char>(0x80 | (c >> 6 & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | c >> 18); out += static_cast<char>(0x80 | (c >> 12 & 0x3F));
            out += static_cast<char>(0x80 | (c >> 6 & 0x3F)); out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return out;
}

std::u16string text(const void* p) {
    if (!p) return std::u16string();
    if (isConst(p)) {
        const ConstString* c = static_cast<const ConstString*>(p);
        if (c->flags == kConstUtf16)
            return std::u16string(static_cast<const char16_t*>(c->str), c->length);
        return fromUtf8(static_cast<const char*>(c->str), c->length);
    }
    if (static_cast<const Object*>(p)->isa == &kStringClass) return static_cast<const String*>(p)->s;
    return describe(p);
}

std::string utf8(const void* p) { return toUtf8(text(p)); }

const void* makeString(const std::u16string& s) { return new String(s); }
const void* makeString(const std::string& s) { return new String(fromUtf8(s.data(), s.size())); }

std::u16string describe(const void* p) {
    if (!p) return u"(null)";
    if (isConst(p)) return text(p);
    const Object* o = static_cast<const Object*>(p);
    if (o->isa && o->isa->describe) return o->isa->describe(o);
    std::string s = std::string("<") + (o->isa ? o->isa->name : "?") + ">";
    return fromUtf8(s.data(), s.size());
}

}
}

using port::cf::text;

extern "C" {

static char s_allocatorNull;
extern const void* const kCFAllocatorDefault = nullptr;
extern const void* const kCFAllocatorNull = &s_allocatorNull;

const void* CFRetain(const void* p) {
    if (p && !isConst(p))
        static_cast<port::cf::Object*>(const_cast<void*>(p))->refs++;
    return p;
}

void CFRelease(const void* p) {
    if (!p || isConst(p)) return;
    port::cf::Object* o = static_cast<port::cf::Object*>(const_cast<void*>(p));
    if (--o->refs == 0) {
        if (o->isa && o->isa->destroy) o->isa->destroy(o);
    }
}

void CFShow(const void* p) {
    fprintf(stderr, "%s\n", port::cf::toUtf8(port::cf::describe(p)).c_str());
}

CFStringEncoding CFStringGetSystemEncoding(void) {
    return kUTF8;
}

const void* CFStringCreateWithCString(const void*, const char* cstr, CFStringEncoding enc) {
    if (!cstr) return nullptr;
    return new String(decode(reinterpret_cast<const unsigned char*>(cstr), strlen(cstr), enc));
}

const void* CFStringCreateWithCStringNoCopy(const void*, const char* cstr, CFStringEncoding enc, const void* deallocator) {
    if (!cstr) return nullptr;
    const void* s = CFStringCreateWithCString(nullptr, cstr, enc);
    if (deallocator == kCFAllocatorDefault)
        free(const_cast<char*>(cstr));
    return s;
}

const void* CFStringCreateWithBytes(const void*, const unsigned char* bytes, CFIndex n, CFStringEncoding enc, bool) {
    if (!bytes && n) return nullptr;
    return new String(decode(bytes, static_cast<size_t>(n), enc));
}

const void* CFStringCreateCopy(const void*, const void* s) { return new String(text(s)); }

const void* CFStringCreateWithSubstring(const void*, const void* s, CFRange r) {
    return new String(text(s).substr(r.location, r.length));
}

const void* CFStringCreateMutable(const void*, CFIndex) { return new String(std::u16string()); }

const void* CFStringCreateWithFormatAndArguments(const void*, const void*, const void* format, va_list ap) {
    std::u16string out;
    formatInto(out, text(format), ap);
    return new String(out);
}

const void* CFStringCreateWithFormat(const void*, const void*, const void* format, ...) {
    va_list ap;
    va_start(ap, format);
    std::u16string out;
    formatInto(out, text(format), ap);
    va_end(ap);
    return new String(out);
}

void CFStringAppend(const void* s, const void* appended) { mut(s)->s += text(appended); }

void CFStringAppendCString(const void* s, const char* cstr, CFStringEncoding enc) {
    if (cstr) mut(s)->s += decode(reinterpret_cast<const unsigned char*>(cstr), strlen(cstr), enc);
}

void CFStringAppendFormat(const void* s, const void*, const void* format, ...) {
    String* m = mut(s);
    va_list ap;
    va_start(ap, format);
    formatInto(m->s, text(format), ap);
    va_end(ap);
}

void CFStringDelete(const void* s, CFRange r) { mut(s)->s.erase(r.location, r.length); }

void CFStringUppercase(const void* s, const void*) {
    String* m = mut(s);
    for (char16_t& c : m->s) c = static_cast<char16_t>(towupper(c));
}

CFIndex CFStringGetLength(const void* s) { return static_cast<CFIndex>(text(s).size()); }

CFIndex CFStringGetCharacterAtIndex(const void* s, CFIndex i) {
    std::u16string t = text(s);
    return i >= 0 && static_cast<size_t>(i) < t.size() ? t[i] : 0;
}

const char16_t* CFStringGetCharactersPtr(const void* s) {
    if (isConst(s)) {
        const ConstString* c = static_cast<const ConstString*>(s);
        return c->flags == kConstUtf16 ? static_cast<const char16_t*>(c->str) : nullptr;
    }
    return port::cf::isString(s) ? static_cast<const String*>(s)->s.c_str() : nullptr;
}

const char* CFStringGetCStringPtr(const void* s, CFStringEncoding enc) {
    if (!s || !(enc == kUTF8 || singleByte(enc))) return nullptr;
    if (isConst(s)) {
        const ConstString* c = static_cast<const ConstString*>(s);
        return c->flags == kConstUtf16 ? nullptr : static_cast<const char*>(c->str);
    }
    if (!port::cf::isString(s)) return nullptr;
    String* m = static_cast<String*>(const_cast<void*>(s));
    if (!m->cstrValid) {
        m->cstr.clear();
        CFIndex n = encode(m->s, 0, m->s.size(), enc, 0, false, m->cstr);
        if (static_cast<size_t>(n) != m->s.size()) return nullptr;
        m->cstrValid = true;
    }
    return m->cstr.c_str();
}

bool CFStringGetCString(const void* s, char* buffer, CFIndex size, CFStringEncoding enc) {
    if (!buffer || size <= 0) return false;
    std::u16string t = text(s);
    std::string out;
    CFIndex n = encode(t, 0, t.size(), enc, 0, false, out);
    if (static_cast<size_t>(n) != t.size() || static_cast<CFIndex>(out.size()) + 1 > size) {
        buffer[0] = 0;
        return false;
    }
    memcpy(buffer, out.data(), out.size());
    buffer[out.size()] = 0;
    return true;
}

CFIndex CFStringGetBytes(const void* s, CFRange r, CFStringEncoding enc, unsigned char lossByte, bool external,
                         unsigned char* buffer, CFIndex maxLen, CFIndex* used) {
    std::u16string t = text(s);
    CFIndex to = r.location + r.length;
    if (to > static_cast<CFIndex>(t.size())) to = t.size();
    std::string out;
    CFIndex chars = encode(t, r.location, to, enc, lossByte, external, out);
    if (buffer && static_cast<CFIndex>(out.size()) > maxLen) {
        std::string part;
        CFIndex c = 0;
        for (; r.location + c < to; c++) {
            std::string one;
            encode(t, r.location + c, r.location + c + 1, enc, lossByte, false, one);
            if (static_cast<CFIndex>(part.size() + one.size()) > maxLen) break;
            part += one;
        }
        out = part;
        chars = c;
    }
    if (buffer) memcpy(buffer, out.data(), out.size());
    if (used) *used = static_cast<CFIndex>(out.size());
    return chars;
}

long CFStringCompare(const void* a, const void* b, unsigned options) { return compare(text(a), text(b), options); }

long CFStringCompareWithOptions(const void* a, const void* b, CFRange r, unsigned options) {
    return compare(text(a).substr(r.location, r.length), text(b), options);
}

CFRange CFStringFind(const void* s, const void* what, unsigned options) {
    std::u16string t = text(s), w = text(what);
    bool ci = options & kCompareCaseInsensitive;
    CFRange none = {kCFNotFound, 0};
    if (w.empty() || w.size() > t.size()) return none;
    if (options & kCompareAnchored) {
        size_t pos = (options & kCompareBackwards) ? t.size() - w.size() : 0;
        return eqAt(t, pos, w, ci) ? CFRange{static_cast<CFIndex>(pos), static_cast<CFIndex>(w.size())} : none;
    }
    if (options & kCompareBackwards) {
        for (size_t p = t.size() - w.size() + 1; p-- > 0;)
            if (eqAt(t, p, w, ci)) return CFRange{static_cast<CFIndex>(p), static_cast<CFIndex>(w.size())};
    } else {
        for (size_t p = 0; p + w.size() <= t.size(); p++)
            if (eqAt(t, p, w, ci)) return CFRange{static_cast<CFIndex>(p), static_cast<CFIndex>(w.size())};
    }
    return none;
}

int CFStringGetIntValue(const void* s) {
    std::string u = port::cf::utf8(s);
    return static_cast<int>(strtol(u.c_str(), nullptr, 10));
}

}

namespace {
struct InlineBuffer {
    unsigned short buffer[64];
    const void* theString;
    const unsigned short* direct;
    int rangeLocation;
    int rangeLength;
    int bufferedRangeStart;
    int bufferedRangeEnd;
};
}

extern "C" unsigned short CFStringGetCharacterFromInlineBuffer(void* buffer, int index) {
    const InlineBuffer* b = static_cast<const InlineBuffer*>(buffer);
    if (!b || index < 0 || index >= b->rangeLength) return 0;
    if (b->direct) return b->direct[b->rangeLocation + index];
    return static_cast<unsigned short>(CFStringGetCharacterAtIndex(b->theString, b->rangeLocation + index));
}
