#pragma once

#include <string>
#include <vector>

namespace port {
namespace platform {
namespace process {

std::string selfPath();
long spawn(const std::vector<std::string>& args, const std::vector<std::string>& env);
int poll(long id);

}
}
}
