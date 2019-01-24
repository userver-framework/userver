#pragma once

#include <string>
#include <thread>

namespace utils {

std::string GetThreadName(std::thread& thread);

std::string GetCurrentThreadName();

void SetThreadName(std::thread& thread, const std::string& name);

void SetCurrentThreadName(const std::string& name);

}  // namespace utils
