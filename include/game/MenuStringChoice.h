#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class WrappedString;
namespace midp {
class String;
}

class MenuStringChoice {
public:
    MenuStringChoice();
    MenuStringChoice(const MenuStringChoice& other);
    virtual ~MenuStringChoice();

    int getSelectedIndex() const;
    int getSelectedItem() const;
    void setSelectedItem(int textId);
    void append(int textId);
    WrappedString& getMenuItemWrappedString(int index);
    midp::String* getThisTitleAllCapsString() const;
    void create(int capacity, int unk48, int titleId);
    void create(int capacity, int titleId);
    void wrapStrings(int font, int width, bool upperCase);
    int getLength() const;
    void setSelectedIndex(int index);
    bool isStringsWrapped();
    int getItem(int index) const;

    midp::array<short> m_items;
    midp::array<midp::ReferenceCountedPointer<midp::String>> m_unk18;
    midp::array<WrappedString> m_wrappedItems;
    int m_length;
    int m_selectedIndex;
    int m_unk48;
    int m_titleId;
    midp::ReferenceCountedPointer<midp::String> m_unk50;
    mutable midp::ReferenceCountedPointer<midp::String> m_titleAllCaps;
};
