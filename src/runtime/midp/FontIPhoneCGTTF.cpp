#include "runtime/midp/FontIPhoneCGFont.h"

#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/String.h"

extern "C" {

unsigned short CFStringGetCharacterFromInlineBuffer(void* buffer, int index);

}

void* operator new(size_t);
void* operator new[](size_t);

namespace midp {

FontIPhoneCGTTF::GlyphMap::GlyphMap() : m_head(0) {}

FontIPhoneCGTTF::GlyphMap::~GlyphMap() {
    Node* node = m_head;
    while (node != 0) {
        Node* next = node->next;
        delete node;
        node = next;
    }
}

int& FontIPhoneCGTTF::GlyphMap::operator[](const int& key) {
    for (Node* node = m_head; node != 0; node = node->next) {
        if (node->key == key)
            return node->value;
    }
    Node* node = new Node;
    node->next = m_head;
    node->key = key;
    node->value = 0;
    m_head = node;
    return node->value;
}

bool FontIPhoneCGTTF::readEncodingRecord(DataInputStream& in, EncodingRecord&, TableRecord&) {
    if (in.readUnsignedShort() != 4)
        return false;
    unsigned length = in.readUnsignedShort();
    in.readUnsignedShort();
    unsigned segCountX2 = in.readUnsignedShort();
    in.readUnsignedShort();
    in.readUnsignedShort();
    in.readUnsignedShort();
    unsigned segCount = (segCountX2 >> 1) & 0x7fff;
    unsigned short* endCode = new unsigned short[segCount];
    unsigned short first = in.readUnsignedShort();
    unsigned short* startCode;
    short* idDelta;
    unsigned short* idRangeOffset;
    if (segCount == 0) {
        startCode = new unsigned short[0];
        idDelta = new short[0];
    } else {
        endCode[0] = first;
        for (unsigned i = 1; i < segCount; i++)
            endCode[i] = static_cast<unsigned short>(in.readUnsignedShort());
        in.readUnsignedShort();
        startCode = new unsigned short[segCount];
        for (unsigned i = 0; i < segCount; i++)
            startCode[i] = static_cast<unsigned short>(in.readUnsignedShort());
        idDelta = new short[segCount];
        for (unsigned i = 0; i < segCount; i++)
            idDelta[i] = in.readShort();
    }
    idRangeOffset = new unsigned short[segCount];
    for (unsigned i = 0; i < segCount; i++)
        idRangeOffset[i] = static_cast<unsigned short>(in.readUnsignedShort());

    int glyphBytes = static_cast<int>(length) - 16 - static_cast<int>(segCountX2) * 4;
    unsigned short* glyphId = new unsigned short[glyphBytes > 0 ? glyphBytes : 0];
    if (glyphBytes > 0) {
        for (int i = 0; i < glyphBytes; i++)
            glyphId[i] = static_cast<unsigned short>(in.readUnsignedShort());
    }

    for (unsigned i = 0; i < segCount; i++) {
        int start = static_cast<short>(startCode[i]);
        int end = static_cast<short>(endCode[i]);
        int delta = idDelta[i];
        unsigned range = idRangeOffset[i];
        if (range == 0) {
            for (int ch = start; ch <= end; ch++)
                m_glyphs[ch] = static_cast<short>(ch + delta);
        } else {
            for (int ch = start; ch <= end; ch++) {
                int index = (ch - start) + static_cast<int>(range >> 1) + static_cast<int>(i) - static_cast<int>(segCount);
                unsigned glyph = glyphId[index];
                if (glyph != 0)
                    m_glyphs[ch] = static_cast<int>(glyph) + delta;
            }
        }
    }

    delete[] endCode;
    delete[] startCode;
    delete[] idDelta;
    delete[] idRangeOffset;
    delete[] glyphId;
    return true;
}

bool FontIPhoneCGTTF::canRenderString(String* str) {
    int length = str->length();
    for (int i = 0; i < length; i++) {
        int ch = str->charAt(i);
        if (m_glyphs[ch] == 0)
            return false;
    }
    return true;
}

int FontIPhoneCGTTF::charToGlyph(int ch) {
    return m_glyphs[ch];
}

void FontIPhoneCGTTF::stringToGlyphs(CFStringInlineBuffer* buffer, unsigned short* glyphs, int count) {
    if (count < 1)
        return;
    for (int i = 0; i < count; i++) {
        int ch = CFStringGetCharacterFromInlineBuffer(buffer, i);
        glyphs[i] = static_cast<unsigned short>(m_glyphs[ch]);
    }
}

FontIPhoneCGTTF::~FontIPhoneCGTTF() {}

FontIPhoneCGTTF::FontIPhoneCGTTF() {}

bool FontIPhoneCGTTF::readCMapTable(DataInputStream& in, TableRecord& table) {
    long base = in.getPosition();
    in.readUnsignedShort();
    unsigned count = in.readUnsignedShort();
    bool found = false;
    for (unsigned i = 0; i < count; i++) {
        in.readUnsignedShort();
        in.readUnsignedShort();
        int offset = in.readInt();
        long here = in.getPosition();
        in.seek(base + offset, 0);
        EncodingRecord encoding;
        found = readEncodingRecord(in, encoding, table) || found;
        in.seek(here, 0);
    }
    return found;
}

bool FontIPhoneCGTTF::readHeader(InputStream* stream) {
    DataInputStream in(stream);
    long base = in.getPosition();
    in.readInt();
    int numTables = in.readUnsignedShort();
    in.readUnsignedShort();
    in.readUnsignedShort();
    in.readUnsignedShort();
    bool found = false;
    for (int i = 0; i < numTables; i++) {
        unsigned tag = static_cast<unsigned>(in.readInt());
        in.readInt();
        int offset = in.readInt();
        in.readInt();
        if (tag != 0x636d6170)
            continue;
        long here = in.getPosition();
        in.seek(base + offset, 0);
        TableRecord table;
        found = readCMapTable(in, table) || found;
        in.seek(here, 0);
    }
    return found;
}

FontIPhoneCGTTF* FontIPhoneCGTTF::createFromStream(InputStream* in) {
    FontIPhoneCGTTF* font = new FontIPhoneCGTTF();
    if (!font->readHeader(in)) {
        delete font;
        return 0;
    }
    return font;
}

}
