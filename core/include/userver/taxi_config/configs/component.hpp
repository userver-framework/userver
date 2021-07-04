#pragma once

/// @file userver/taxi_config/configs/component.hpp
/// @brief @copybrief components::TaxiConfigClient

#include <userver/clients/config/client.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Component that starts a clients::taxi_config::Client client.
///
/// Returned references to clients::taxi_config::Client live for a lifetime
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
/// config-url | HTTP URL to request configs via POST request, ignored if use-uconfigs is true | -
/// use-uconfigs | set to true to read stage name from "/etc/yandex/settings.json" and send it in requests | false
/// uconfigs-url | HTTP URL to request configs via POST request if use-uconfigs is true | -
/// fallback-to-no-proxy | make additional attempts to retrieve configs by bypassing proxy that is set in USERVER_HTTP_PROXY runtime variable | true
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi configs client component config

// clang-format on
class TaxiConfigClient : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "taxi-configs-client";

  TaxiConfigClient(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfigClient() override = default;

  [[nodiscard]] clients::taxi_config::Client& GetClient() const;

 private:
  std::unique_ptr<clients::taxi_config::Client> config_client_;
};

}  // namespace components
