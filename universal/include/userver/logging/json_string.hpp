#pragma once

/// @file userver/logging/json_string.hpp
/// @brief @copybrief logging::JsonString

#include <userver/formats/json/raw_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief JSON stored as string
/// @see formats::json::RawString
using JsonString = formats::json::RawString;

}  // namespace logging

USERVER_NAMESPACE_END
