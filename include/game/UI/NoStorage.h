#pragma once

#include "game/UI/ConfirmationWindow.h"

namespace UI {

class NoStorage : public ConfirmationWindow {
public:
    NoStorage();
    virtual ~NoStorage();
};

}
