#pragma once

#include <string>

#include <spdlog/common.h>

#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

const std::string& GetSpdlogPattern(Format format);

spdlog::level::level_enum ToSpdlogLevel(Level level) noexcept;

}  // namespace logging

USERVER_NAMESPACE_END
