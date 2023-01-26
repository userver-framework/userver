#include <logging/spdlog_helpers.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

const std::string& GetSpdlogPattern(Format format) {
  static const std::string kSpdlogTskvPattern =
      "tskv\ttimestamp=%Y-%m-%dT%H:%M:%S.%f\tlevel=%l\t%v";
  static const std::string kSpdlogLtsvPattern =
      "timestamp:%Y-%m-%dT%H:%M:%S.%f\tlevel:%l\t%v";
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

}  // namespace logging

USERVER_NAMESPACE_END
