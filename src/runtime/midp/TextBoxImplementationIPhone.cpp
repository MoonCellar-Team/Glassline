#include "runtime/midp/TextBoxImplementationIPhone.h"

#include <stddef.h>

#include "runtime/midp/String.h"
#include "runtime/midp/TextBox.h"
#include "sdk/Foundation.h"

namespace {

class TextBoxDelegate : public NSObject {
public:
    static TextBoxDelegate* alloc();
    TextBoxDelegate* initWithImplementation(midp::TextBoxImplementationIPhone* impl);
};

class UITextField : public NSObject {
public:
    NSString* text();
};

}

namespace midp {

void TextBoxImplementationIPhone::hideNotify() {
    static_cast<NSObject*>(m_delegate)->performSelectorOnMainThread_withObject_waitUntilDone(
        sel_registerName("hideNotify:"), m_field, false);
}

void TextBoxImplementationIPhone::showNotify() {
    static_cast<NSObject*>(m_delegate)->performSelectorOnMainThread_withObject_waitUntilDone(
        sel_registerName("showNotify:"), m_field, false);
}

TextBoxImplementationIPhone::TextBoxImplementationIPhone(TextBox* textBox)
    : TextBoxImplementation(textBox), m_delegate(NULL), m_field(NULL) {
    TextBoxDelegate* delegate = TextBoxDelegate::alloc()->initWithImplementation(this);
    m_delegate = delegate;
    delegate->performSelectorOnMainThread_withObject_waitUntilDone(
        sel_registerName("createTextField"), NULL, true);
}

TextBoxImplementationIPhone::~TextBoxImplementationIPhone() {
    NSObject* delegate = static_cast<NSObject*>(m_delegate);
    delegate->performSelectorOnMainThread_withObject_waitUntilDone(sel_registerName("remove:"),
                                                                  m_field, false);
    delegate->release();
}

void TextBoxImplementationIPhone::endEditing() {
    NSString* raw = static_cast<UITextField*>(m_field)->text();
    NSString* trimmed =
        raw->stringByTrimmingCharactersInSet(NSCharacterSet::whitespaceCharacterSet());
    trimmed->retain();
    String* text = new String(reinterpret_cast<const __CFString*>(trimmed));
    getTextBox()->setString(text);
    getTextBox()->returnToGame();
}

}
