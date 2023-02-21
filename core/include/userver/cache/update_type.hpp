#pragma once

/// @file userver/cache/update_type.hpp
/// @brief Enums representing periodic update types for `CachingComponentBase`

#include <string_view>

#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// Type of `CachingComponentBase` update
enum class UpdateType {
  kFull,         ///< requests all the items, starting from scratch
  kIncremental,  ///< requests only newly updated items
};

UpdateType Parse(const formats::json::Value& value,
                 formats::parse::To<UpdateType>);

std::string_view ToString(UpdateType update_type);

/// Update types allowed for a `CachingComponentBase` instance by static config
enum class AllowedUpdateTypes {
  kFullAndIncremental,
  kOnlyFull,
  kOnlyIncremental,
};

AllowedUpdateTypes Parse(const yaml_config::YamlConfig& value,
                         formats::parse::To<AllowedUpdateTypes>);

std::string_view ToString(AllowedUpdateTypes allowed_update_types);

}  // namespace cache

USERVER_NAMESPACE_END
