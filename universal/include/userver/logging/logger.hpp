#pragma once

// TODO: remove this after TAXICOMMON-1949

#include <any>
#include <cerrno>
#include <initializer_list>
#include <string>
#include <utility>

#include <logging/log.hpp>

struct LogExtra {
  LogExtra() = default;
  LogExtra(std::initializer_list<std::pair<std::string, std::any>>) {}

  template <typename... Args>
  void Extend(Args&&...) {}

  template <typename... Args>
  void ExtendRange(Args&&...) {}
};

namespace logging {

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

}  // namespace logging
