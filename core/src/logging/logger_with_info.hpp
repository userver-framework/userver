#pragma once

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <userver/logging/format.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class LoggerWithInfo final {
 public:
  LoggerWithInfo(Format format, utils::SharedRef<spdlog::logger> ptr)
      : format(format), ptr(std::move(ptr)) {}

  const Format format;
  const utils::SharedRef<spdlog::logger> ptr;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
