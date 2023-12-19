#pragma once

/// @file userver/logging/format.hpp
/// @brief Log formats

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// Log formats
enum class Format { kTskv, kLtsv, kRaw };

/// Parse Format enum from string
Format FormatFromString(std::string_view format_str);

}  // namespace logging

USERVER_NAMESPACE_END
