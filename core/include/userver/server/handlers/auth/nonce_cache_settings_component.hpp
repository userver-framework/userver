#pragma once

/// @file userver/server/handlers/auth/nonce_cache_settings_component.hpp
/// @brief @copybrief server::handlers::auth::NonceCacheSettingsComponent

#include <cstddef>
#include <chrono>
#include <optional>
#include <string>

#include <userver/server/handlers/auth/digest_checker_settings_component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

#include "auth_digest_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

struct NonceCacheSettings {
  std::size_t ways{};
  std::size_t way_size{};
};

// clang-format off

/// @brief Component that loads nonce cache settings from a static_config.yaml
/// ## Static options:
///
/// Name   | Description                           | Default value
/// ------ | --------------------------------------| -------------
/// size   | max amount of items to store in cache | -
/// ways   | number of ways for associative cache  | -

// clang-format on

class NonceCacheSettingsComponent final
    : public DigestCheckerSettingsComponent {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  /// server::handlers::auth::NonceCacheSettingsComponent
  static constexpr std::string_view kName = "nonce-cache-settings";

  NonceCacheSettingsComponent(const components::ComponentConfig& config,
                              const components::ComponentContext& context);

  ~NonceCacheSettingsComponent() final;

  const NonceCacheSettings& GetSettings() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  NonceCacheSettings settings_;
};

}  // namespace server::handlers::auth

template <>
inline constexpr bool components::kHasValidate<
    server::handlers::auth::NonceCacheSettingsComponent> = true;

USERVER_NAMESPACE_END
