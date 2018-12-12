#pragma once

#include <chrono>
#include <string>

namespace utils {

std::chrono::milliseconds StringToDuration(const std::string& data);

}  // namespace utils
