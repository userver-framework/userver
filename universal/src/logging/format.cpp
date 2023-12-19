#include <userver/logging/format.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

Format FormatFromString(std::string_view format_str) {
  if (format_str == "tskv") {
    return Format::kTskv;
  }

  if (format_str == "ltsv") {
    return Format::kLtsv;
  }

  if (format_str == "raw") {
    return Format::kRaw;
  }

  UINVARIANT(
      false,
      fmt::format("Unknown logging format '{}' (must be one of 'tskv', 'ltsv')",
                  format_str));
}

}  // namespace logging

USERVER_NAMESPACE_END
