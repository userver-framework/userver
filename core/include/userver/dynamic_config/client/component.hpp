#pragma once

/// @file userver/dynamic_config/client/component.hpp
/// @brief @copybrief components::DynamicConfigClient

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/client/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that starts a clients::dynamic_config::Client client.
///
/// Returned references to clients::dynamic_config::Client live for a lifetime
/// of the component and are safe for concurrent use.
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// get-configs-overrides-for-service | send service-name field | true
/// service-name | name of the service to send if the get-configs-overrides-for-service is true | -
/// http-timeout | HTTP request timeout to the remote in utils::StringToDuration() suitable format | -
/// http-retries | HTTP retries before reporting the request failure | -
/// config-url | HTTP URL to request configs via POST request | -
/// append-path-to-url | add default path '/configs/values' to 'config-url' | true
/// configs-stage: stage name provided statically, can be overridden from file | -
/// configs-stage-filepath: file to read stage name from, overrides static "configs-stage" if both are provided, expected format: json file with "env_name" property | -
/// fallback-to-no-proxy | make additional attempts to retrieve configs by bypassing proxy that is set in USERVER_HTTP_PROXY runtime variable | true
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample dynamic configs client component config

// clang-format on
class DynamicConfigClient : public LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::DynamicConfigClient
  static constexpr std::string_view kName = "dynamic-config-client";

  DynamicConfigClient(const ComponentConfig&, const ComponentContext&);
  ~DynamicConfigClient() override = default;

  [[nodiscard]] dynamic_config::Client& GetClient() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::unique_ptr<dynamic_config::Client> config_client_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfigClient> = true;

}  // namespace components

USERVER_NAMESPACE_END
