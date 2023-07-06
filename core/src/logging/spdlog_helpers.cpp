#include <logging/spdlog_helpers.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

const std::string& GetSpdlogPattern(Format format) {
  static const std::string kSpdlogTskvPattern =
      "tskv\ttimestamp=%Y-%m-%dT%H:%M:%S.%f\tlevel=%l%v";
  static const std::string kSpdlogLtsvPattern =
      "timestamp:%Y-%m-%dT%H:%M:%S.%f\tlevel:%l%v";
  static const std::string kSpdlogRawPattern = "%v";

  switch (format) {
    case Format::kTskv:
      return kSpdlogTskvPattern;
    case Format::kLtsv:
      return kSpdlogLtsvPattern;
    case Format::kRaw:
      return kSpdlogRawPattern;
  }

  UINVARIANT(false, "Invalid logging::Format enum value");
}

spdlog::level::level_enum ToSpdlogLevel(Level level) noexcept {
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

  return static_cast<spdlog::level::level_enum>(level);
}

}  // namespace logging

USERVER_NAMESPACE_END
