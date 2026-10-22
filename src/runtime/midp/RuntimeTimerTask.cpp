#include "runtime/midp/RuntimeTimerTask.h"

#include "runtime/midp/Manager.h"

namespace midp {

RuntimeTimerTask::RuntimeTimerTask() {}

RuntimeTimerTask::~RuntimeTimerTask() {}

void RuntimeTimerTask::run() {
    Manager::update();
}

}
