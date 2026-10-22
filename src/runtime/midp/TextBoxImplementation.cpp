#include "runtime/midp/TextBoxImplementation.h"

namespace midp {

TextBoxImplementation::TextBoxImplementation(TextBox* textBox) : m_textBox(textBox) {}

TextBoxImplementation::~TextBoxImplementation() {}

TextBox* TextBoxImplementation::getTextBox() const {
    return m_textBox;
}

}
