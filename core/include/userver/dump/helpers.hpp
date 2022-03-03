#pragma once

/// @file userver/dump/helpers.hpp
/// @brief Dump utils

#include <chrono>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace dump {

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

[[noreturn]] void ThrowDumpUnimplemented(const std::string& name);

}  // namespace dump

USERVER_NAMESPACE_END
