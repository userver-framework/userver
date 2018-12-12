#pragma once

#include <string>
#include <thread>

namespace utils {

std::string GetThreadName(std::thread& thread);
void SetThreadName(std::thread& thread, const std::string& name);

}  // namespace utils
