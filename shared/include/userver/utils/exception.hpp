#pragma once

/// @file userver/utils/exception.hpp
/// @brief @copybrief utils::LogErrorAndThrow

#include <stdexcept>
#include <string>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Logs error_message and throws an exception ith that message
template <typename T = std::runtime_error>
[[noreturn]] void LogErrorAndThrow(const std::string& error_message) {
  LOG_ERROR() << error_message;
  throw T(error_message);
}

}  // namespace utils

USERVER_NAMESPACE_END
