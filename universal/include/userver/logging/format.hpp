#pragma once

/// @file userver/logging/format.hpp
/// @brief Log formats

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief Text-based log formats
///
/// For otlp logs, see @ref scripts/docs/en/userver/logging.md
enum class Format {
    kTskv,
    kLtsv,
    kRaw,
    kJson,
    kJsonYaDeploy,
};

/// Parse Format enum from string
Format FormatFromString(std::string_view format_str);

}  // namespace logging

USERVER_NAMESPACE_END
