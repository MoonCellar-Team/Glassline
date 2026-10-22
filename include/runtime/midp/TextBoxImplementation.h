#pragma once

namespace midp {

class TextBox;

class TextBoxImplementation {
public:
    explicit TextBoxImplementation(TextBox* textBox);
    virtual ~TextBoxImplementation();
    virtual void showNotify() = 0;
    virtual void hideNotify() = 0;

    TextBox* getTextBox() const;

    TextBox* m_textBox;
};

}
