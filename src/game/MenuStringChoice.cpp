#include "game/MenuStringChoice.h"

#include <stddef.h>

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "game/WrappedString.h"
#include "runtime/midp/String.h"

static const int TEXT_NONE = 0x800;

int MenuStringChoice::getSelectedIndex() const {
    return m_selectedIndex;
}

MenuStringChoice::MenuStringChoice()
    : m_length(0), m_selectedIndex(0), m_unk48(TEXT_NONE), m_titleId(TEXT_NONE), m_unk50(NULL), m_titleAllCaps(NULL) {}

int MenuStringChoice::getSelectedItem() const {
    if (m_items.m_length == 0) {
        return m_titleId;
    }
    return m_items[m_selectedIndex];
}

void MenuStringChoice::setSelectedItem(int textId) {
    int count = m_items.m_length;
    for (int i = 0; i != count; i++) {
        if (m_items[i] == textId) {
            m_selectedIndex = i;
            return;
        }
    }
}

void MenuStringChoice::append(int textId) {
    m_items[m_length++] = textId;
}

WrappedString& MenuStringChoice::getMenuItemWrappedString(int index) {
    return m_wrappedItems[index];
}

midp::String* MenuStringChoice::getThisTitleAllCapsString() const {
    if (m_titleAllCaps == NULL) {
        m_titleAllCaps = AppEngine::getCanvas()->getTextManager()->getString(m_titleId)->toUpperCase();
    }
    return m_titleAllCaps;
}

MenuStringChoice::~MenuStringChoice() {}

MenuStringChoice::MenuStringChoice(const MenuStringChoice& other)
    : m_items(other.m_items),
      m_unk18(other.m_unk18),
      m_wrappedItems(other.m_wrappedItems),
      m_length(other.m_length),
      m_selectedIndex(other.m_selectedIndex),
      m_unk48(other.m_unk48),
      m_titleId(other.m_titleId),
      m_unk50(other.m_unk50),
      m_titleAllCaps(other.m_titleAllCaps) {}

void MenuStringChoice::create(int capacity, int unk48, int titleId) {
    if (m_items.m_length != capacity) {
        m_items = midp::array<short>(capacity);
    }
    m_unk18 = midp::array<midp::ReferenceCountedPointer<midp::String>>();
    m_wrappedItems = midp::array<WrappedString>();
    m_unk50 = NULL;
    m_titleAllCaps = NULL;
    m_length = 0;
    m_selectedIndex = 0;
    m_unk48 = unk48;
    m_titleId = titleId;
}

void MenuStringChoice::create(int capacity, int titleId) {
    create(capacity, TEXT_NONE, titleId);
}

void MenuStringChoice::wrapStrings(int font, int width, bool upperCase) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_wrappedItems = midp::array<WrappedString>(m_length);
    for (int i = 0; i != m_length; i++) {
        midp::ReferenceCountedPointer<midp::String> str = text->getString(m_items[i]);
        if (upperCase) {
            str = str->toUpperCase();
        }
        m_wrappedItems[i].wrapString(str, font, width);
    }
}

int MenuStringChoice::getLength() const {
    return m_length;
}

void MenuStringChoice::setSelectedIndex(int index) {
    m_selectedIndex = index;
}

bool MenuStringChoice::isStringsWrapped() {
    return m_wrappedItems.m_length != 0;
}

int MenuStringChoice::getItem(int index) const {
    return m_items[index];
}
