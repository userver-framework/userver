#pragma once

#include <string>

namespace utils {

std::string GetCurrentThreadName();
void SetCurrentThreadName(const std::string& name);

}  // namespace utils
