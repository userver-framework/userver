#pragma once

#include <server_settings/http_server_settings_base_component.hpp>
#include <server_settings/http_server_settings_config.hpp>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/value.hpp>

namespace components {

template <typename HttpServerSettingsConfigNames =
              server_settings::ConfigNamesDefault>
class HttpServerSettings : public HttpServerSettingsBase {
 public:
  HttpServerSettings(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
      : HttpServerSettingsBase(config, context),
        taxi_config_component_(
            context.FindComponent<components::TaxiConfig>()) {}
  virtual ~HttpServerSettings() = default;

  bool NeedLogRequest() const override;
  bool NeedLogRequestHeaders() const override;

 private:
  auto GetHttpServerSettingsConfig() const;

  const components::TaxiConfig& taxi_config_component_;
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
auto HttpServerSettings<
    HttpServerSettingsConfigNames>::GetHttpServerSettingsConfig() const {
  auto taxi_config = taxi_config_component_.Get();
  return taxi_config->template Get<server_settings::HttpServerSettingsConfig<
      HttpServerSettingsConfigNames>>();
}

}  // namespace components
