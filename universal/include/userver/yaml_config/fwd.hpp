#pragma once

/// @file userver/yaml_config/fwd.hpp
/// @brief Forward declarations of YamlConfig type and formats::parse::To.
/// @ingroup userver_universal

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

class YamlConfig;

using Value = YamlConfig;

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
struct Schema;

}  // namespace yaml_config

USERVER_NAMESPACE_END
