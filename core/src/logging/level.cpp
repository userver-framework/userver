#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>

#include <functional>
#include <stdexcept>

#include <fmt/format.h>

#include <logging/spdlog.hpp>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/trivial_map.hpp>

namespace std {

template <>
struct hash<USERVER_NAMESPACE::logging::Level> {
  std::size_t operator()(USERVER_NAMESPACE::logging::Level level) const {
    return static_cast<std::size_t>(level);
  }
};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace logging {

static_assert(static_cast<spdlog::level::level_enum>(Level::kTrace) ==
                  spdlog::level::trace,
              "kTrace enumerator value doesn't match spdlog's trace");
static_assert(static_cast<spdlog::level::level_enum>(Level::kDebug) ==
                  spdlog::level::debug,
              "kDebug enumerator value doesn't match spdlog's debug");
static_assert(static_cast<spdlog::level::level_enum>(Level::kInfo) ==
                  spdlog::level::info,
              "kInfo enumerator value doesn't match spdlog's info");
static_assert(static_cast<spdlog::level::level_enum>(Level::kWarning) ==
                  spdlog::level::warn,
              "kWarning enumerator value doesn't match spdlog's warn");
static_assert(static_cast<spdlog::level::level_enum>(Level::kError) ==
                  spdlog::level::err,
              "kError enumerator value doesn't match spdlog's err");
static_assert(static_cast<spdlog::level::level_enum>(Level::kCritical) ==
                  spdlog::level::critical,
              "kCritical enumerator value doesn't match spdlog's critical");
static_assert(static_cast<spdlog::level::level_enum>(Level::kNone) ==
                  spdlog::level::off,
              "kNone enumerator value doesn't match spdlog's off");

namespace {

constexpr utils::TrivialBiMap kLevelMap = [](auto selector) {
  return selector()
      .Case("trace", Level::kTrace)
      .Case("debug", Level::kDebug)
      .Case("info", Level::kInfo)
      .Case("warning", Level::kWarning)
      .Case("error", Level::kError)
      .Case("critical", Level::kCritical)
      .Case("none", Level::kNone);
};

}  // namespace

Level LevelFromString(std::string_view level_name) {
  auto value = kLevelMap.TryFindICase(level_name);
  if (!value) {
    throw std::runtime_error(
        fmt::format("Unknown log level '{}' (must be one of {})", level_name,
                    kLevelMap.DescribeFirst()));
  }
  return *value;
}

std::string ToString(Level level) {
  auto value = kLevelMap.TryFind(level);
  if (!value) {
    return "Unknown (" + std::to_string(static_cast<int>(level)) + ')';
  }
  return std::string{*value};
}

std::optional<Level> OptionalLevelFromString(
    const std::optional<std::string>& level_name) {
  if (level_name)
    return LevelFromString(*level_name);
  else
    return std::nullopt;
}

bool ShouldLogNospan(Level level) noexcept {
  return impl::DefaultLoggerRef().ShouldLog(level);
}

bool ShouldLog(Level level) noexcept {
  if (!ShouldLogNospan(level)) return false;

  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) {
    auto local_log_level = span->GetLocalLogLevel();
    if (local_log_level && *local_log_level > level) {
      return false;
    }
  }

  return true;
}

}  // namespace logging

USERVER_NAMESPACE_END
