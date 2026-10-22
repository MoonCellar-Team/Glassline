#include "runtime/midp/Font.h"

#include "runtime/midp/FontClass.h"
#include "runtime/midp/FontIPhone.h"
#include "runtime/midp/FontIPhoneCGFont.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "sdk/CoreGraphics.h"
#include "sdk/UIKit.h"

namespace midp {

Class* Font::getClass() const {
    return new FontClass();
}

Font::~Font() {}

Font::Font() {}

void Font::getLayoutPadding(int& left, int& right, int& top, int& bottom) {
    left = 0;
    right = 0;
    top = 0;
    bottom = 0;
}

bool Font::canRenderString(String*) {
    return true;
}

bool Font::canRenderChar(unsigned short) {
    return true;
}

Font* Font::getFont(InputStream* in, float size) {
    INCREF(in);
    in->mark(0);
    FontIPhoneCGTTF* ttf = FontIPhoneCGTTF::createFromStream(in);
    in->reset();
    void* provider = CGDataProviderCreateWithInputStream(in);
    CGFont* cg = CGFontCreateWithDataProvider(provider);
    Font* font = new FontIPhoneCGFont(cg, size, ttf);
    CGFontRelease(cg);
    CGDataProviderRelease(provider);
    DECREF(in);
    return font;
}

struct CGAffineTransform {
    float a;
    float b;
    float c;
    float d;
    float tx;
    float ty;
};

static CGAffineTransform __CGAffineTransformMake(float, float, float, float, float, float) {
    CGAffineTransform t;
    t.a = 1.0f;
    t.b = 0.0f;
    t.c = 0.0f;
    t.d = -1.0f;
    t.tx = 0.0f;
    t.ty = 0.0f;
    return t;
}

Font* Font::getDefaultFont() {
    float size = UIFont::systemFontSize();
    UIFont* font = UIFont::systemFontOfSize(size);
    return new FontIPhoneUIFont(font);
}

Font* Font::getFont(ReferenceCountedPointer<String> name, float size) {
    UIFont* font = UIFont::fontWithName_size((NSString*)name->getCFString(), size);
    return new FontIPhoneUIFont(font);
}

}
