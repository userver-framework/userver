#include <userver/logging/format.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include "userver/utils/trivial_map.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

Format FormatFromString(std::string_view format_str) {

  constexpr utils::TrivialBiMap kFormatToStr = [](auto selector) {
    return selector()
        .Case(Format::kTskv, "tskv")
        .Case(Format::kLtsv, "ltsv")
        .Case(Format::kRaw, "raw")
        .Case(Format::kJson, "json");
  };

  auto format_enum = kFormatToStr.TryFind(format_str);
  if (format_enum.has_value()) {
    return format_enum.value();
  }

  auto expected_formats = kFormatToStr.DescribeSecond();

  UINVARIANT(
      false,
      fmt::format("Unknown logging format '{}' (must be one of {})",
                  format_str, expected_formats));
}

}  // namespace logging

USERVER_NAMESPACE_END
