#pragma once

#include <string>

namespace blocking::system {

/// Returns host name. Warning: Blocking function!
std::string GetRealHostName();

}  // namespace blocking::system
