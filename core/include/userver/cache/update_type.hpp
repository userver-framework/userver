#pragma once

/// @file userver/cache/update_type.hpp
/// @brief Enums representing periodic update types for `CachingComponentBase`

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// Type of `CachingComponentBase` update
enum class UpdateType {
  kFull,         ///< requests all the items, starting from scratch
  kIncremental,  ///< requests only newly updated items
};

/// Update types allowed for a `CachingComponentBase` instance by static config
enum class AllowedUpdateTypes {
  kFullAndIncremental,
  kOnlyFull,
  kOnlyIncremental,
};

AllowedUpdateTypes Parse(const yaml_config::YamlConfig& value,
                         formats::parse::To<AllowedUpdateTypes>);

}  // namespace cache

USERVER_NAMESPACE_END
