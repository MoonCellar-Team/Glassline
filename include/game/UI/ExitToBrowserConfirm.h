#pragma once

#include "game/UI/ConfirmationWindow.h"

namespace UI {

class ExitToBrowserConfirm : public ConfirmationWindow {
public:
    ExitToBrowserConfirm();
    virtual ~ExitToBrowserConfirm();
};

}
