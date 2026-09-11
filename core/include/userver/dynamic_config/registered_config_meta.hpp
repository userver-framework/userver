#pragma once

/// @file userver/dynamic_config/registered_config_meta.hpp
/// @brief @copybrief dynamic_config::RegisteredConfigMeta

#include <string_view>

#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// @brief Metadata of a single registered dynamic config item.
/// @note All fields reference storage owned by the global registry. Obtain
/// metadata only after static initialization completes; the views remain valid
/// until the registry is destroyed.
struct RegisteredConfigMeta final {
    std::string_view name;
    /// Opaque schema hash. Empty if the registering constructor did not provide
    /// schema metadata.
    std::string_view schema_hash;
    /// Default value of this config item as a JSON string.
    std::string_view default_as_json_string{};
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
