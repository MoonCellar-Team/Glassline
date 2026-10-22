#pragma once

#include "runtime/midp/Displayable.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"

namespace midp {

class Display;
class String;
class TextBoxImplementation;

class TextBox : public Displayable {
public:
    TextBox(String* title, String* text, int maxSize, int constraints, int flags);
    virtual ~TextBox();
    virtual Class* getClass() const;
    String* getString() const;
    String* getTitle() const;
    void setConstraints(int constraints);
    void setFlags(int flags);
    void setMaxSize(int maxSize);
    void setString(String* text);
    void setTitle(String* title);
    virtual void showNotify();
    virtual void hideNotify();
    void returnToGame();
    void show(Display* display);
    static TextBoxImplementation* getImplementation(TextBox* textBox);

    String* m_title;
    String* m_text;
    int m_maxSize;
    int m_constraints;
    int m_flags;
    TextBoxImplementation* m_impl;
    SafeReferenceCountedPointer<Displayable> m_previous;
};

}
