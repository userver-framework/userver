#pragma once

#include <yandex/taxi/userver/server/request/http_server_settings_base_component.hpp>
#include <yandex/taxi/userver/taxi_config/http_server_settings_config.hpp>
#include <yandex/taxi/userver/taxi_config/value.hpp>

#include "component.hpp"

namespace components {

class HttpServerSettings : public HttpServerSettingsBase {
 public:
  HttpServerSettings(const components::ComponentConfig&,
                     const components::ComponentContext& context)
      : taxi_config_component_(
            context.FindComponent<components::TaxiConfig>()) {}
  virtual ~HttpServerSettings() = default;

  bool NeedLogRequest() const override;
  bool NeedLogRequestHeaders() const override;

 private:
  const components::TaxiConfig* taxi_config_component_ = nullptr;
};

bool HttpServerSettings::NeedLogRequest() const {
  auto taxi_config = taxi_config_component_->Get();
  return taxi_config->Get<server::HttpServerSettingsConfig>().need_log_request;
}

bool HttpServerSettings::NeedLogRequestHeaders() const {
  auto taxi_config = taxi_config_component_->Get();
  return taxi_config->Get<server::HttpServerSettingsConfig>()
      .need_log_request_headers;
}

}  // namespace components
