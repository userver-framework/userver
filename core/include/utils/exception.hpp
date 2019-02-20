#pragma once

#include <stdexcept>
#include <string>

#include <logging/log.hpp>

namespace utils {

template <typename T = std::runtime_error>
[[noreturn]] void LogErrorAndThrow(const std::string& error_message) {
  LOG_ERROR() << error_message;
  throw T(error_message);
}

}  // namespace utils
