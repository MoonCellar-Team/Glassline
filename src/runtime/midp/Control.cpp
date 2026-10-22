#include "runtime/midp/Control.h"

#include "runtime/midp/ControlClass.h"

namespace midp {

Class* Control::getClass() const {
    return new ControlClass();
}

Control::Control() {}

Control::~Control() {}

}
