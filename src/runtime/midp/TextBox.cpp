#include "runtime/midp/TextBox.h"

#include <stddef.h>

#include "runtime/midp/Display.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "runtime/midp/TextBoxImplementationIPhone.h"
#include "sdk/CoreGraphics.h"

namespace midp {

Class* TextBox::getClass() const {
    return NULL;
}

String* TextBox::getString() const {
    return m_text;
}

String* TextBox::getTitle() const {
    return m_title;
}

void TextBox::setConstraints(int constraints) {
    m_constraints = constraints;
}

void TextBox::setFlags(int flags) {
    m_flags = flags;
}

void TextBox::setMaxSize(int maxSize) {
    m_maxSize = maxSize;
}

void TextBox::setString(String* text) {
    INCREF(text);
    DECREF(m_text);
    m_text = text;
}

void TextBox::setTitle(String* title) {
    INCREF(title);
    DECREF(m_title);
    m_title = title;
}

void TextBox::showNotify() {
    m_impl->showNotify();
}

void TextBox::hideNotify() {
    m_impl->hideNotify();
}

void TextBox::returnToGame() {
    Display* display = getDisplay();
    display->setCurrent(m_previous.unsafeGetWrappedPointer());
    m_previous.reset();
}

void TextBox::show(Display* display) {
    m_previous.reset(display->getCurrent());
    display->setCurrent(this);
}

TextBox::~TextBox() {
    if (m_impl != NULL)
        m_impl->~TextBoxImplementation();
    setTitle(NULL);
    setString(NULL);
}

TextBox::TextBox(String* title, String* text, int maxSize, int constraints, int flags)
    : m_title(NULL),
      m_text(NULL),
      m_maxSize(0),
      m_constraints(0),
      m_flags(flags),
      m_impl(NULL),
      m_previous() {
    setTitle(title);
    setString(text);
    setMaxSize(maxSize);
    setFlags(flags);
    setConstraints(constraints);
    m_impl = getImplementation(this);
}

TextBoxImplementation* TextBox::getImplementation(TextBox* textBox) {
    return new TextBoxImplementationIPhone(textBox);
}

}

CGPoint CGPointMake(float x, float) {
    CGPoint point;
    point.x = x;
    point.y = 512.0f;
    return point;
}

CGRect CGRectMake(float x, float y, float width, float height) {
    CGRect rect;
    rect.origin.x = x;
    rect.origin.y = y;
    rect.size.width = width;
    rect.size.height = height;
    return rect;
}
