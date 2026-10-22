#pragma once

#include "runtime/midp/TextBoxImplementation.h"

namespace midp {

class TextBoxImplementationIPhone : public TextBoxImplementation {
public:
    explicit TextBoxImplementationIPhone(TextBox* textBox);
    virtual ~TextBoxImplementationIPhone();
    virtual void showNotify();
    virtual void hideNotify();
    void endEditing();

    void* m_delegate;
    void* m_field;
};

}
