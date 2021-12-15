#pragma once

/// @file userver/logging/level_serialization.hpp
/// @brief Serialization of log levels
/// @ingroup userver_formats_parse

#include <userver/logging/level.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

Level Parse(const yaml_config::YamlConfig& value, formats::parse::To<Level>);

}  // namespace logging

USERVER_NAMESPACE_END
