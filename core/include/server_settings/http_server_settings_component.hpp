#pragma once

/// @file server_settings/http_server_settings_component.hpp
/// @brief @copybrief components::HttpServerSettings

#include <server/handlers/auth/auth_checker_settings_component.hpp>
#include <server_settings/http_server_settings_base_component.hpp>
#include <server_settings/http_server_settings_config.hpp>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/value.hpp>

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Component that initializes the request tracing facilities.
///
/// Finds the components::Logging component and requests an optional
/// "opentracing" logger to use for Opentracing.
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// service-name | service name to write in traces | -
/// tracer | tracer type, currently supported only 'native' | 'native'

// clang-format on
template <typename HttpServerSettingsConfigNames =
              server_settings::ConfigNamesDefault>
class HttpServerSettings final : public HttpServerSettingsBase {
 public:
  HttpServerSettings(const ComponentConfig& config,
                     const ComponentContext& context)
      : HttpServerSettingsBase(config, context),
        taxi_config_component_(context.FindComponent<TaxiConfig>()),
        auth_checker_settings_component_(
            context.FindComponent<AuthCheckerSettings>()) {}
  ~HttpServerSettings() override = default;

  bool NeedLogRequest() const override;
  bool NeedLogRequestHeaders() const override;
  bool NeedCheckAuthInHandlers() const override;

  const server::handlers::auth::AuthCheckerSettings& GetAuthCheckerSettings()
      const override;

 private:
  auto GetHttpServerSettingsConfig() const;

  const TaxiConfig& taxi_config_component_;
  const AuthCheckerSettings& auth_checker_settings_component_;
};

template <typename HttpServerSettingsConfigNames>
bool HttpServerSettings<HttpServerSettingsConfigNames>::NeedLogRequest() const {
  return GetHttpServerSettingsConfig().need_log_request;
}

template <typename HttpServerSettingsConfigNames>
bool HttpServerSettings<HttpServerSettingsConfigNames>::NeedLogRequestHeaders()
    const {
  return GetHttpServerSettingsConfig().need_log_request_headers;
}

template <typename HttpServerSettingsConfigNames>
bool HttpServerSettings<
    HttpServerSettingsConfigNames>::NeedCheckAuthInHandlers() const {
  return GetHttpServerSettingsConfig().need_check_auth_in_handlers;
}

template <typename HttpServerSettingsConfigNames>
const server::handlers::auth::AuthCheckerSettings& HttpServerSettings<
    HttpServerSettingsConfigNames>::GetAuthCheckerSettings() const {
  return auth_checker_settings_component_.Get();
}

template <typename HttpServerSettingsConfigNames>
auto HttpServerSettings<
    HttpServerSettingsConfigNames>::GetHttpServerSettingsConfig() const {
  auto taxi_config = taxi_config_component_.Get();
  return taxi_config->template Get<server_settings::HttpServerSettingsConfig<
      HttpServerSettingsConfigNames>>();
}

}  // namespace components
