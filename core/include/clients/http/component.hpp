#pragma once

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use components::Http from clients/http.hpp instead
#endif

#include <components/component_base.hpp>
#include <taxi_config/config.hpp>

#include <components/statistics_storage.hpp>
#include <utils/async_event_channel.hpp>

namespace clients::http {
class Client;
}  // namespace clients::http

namespace components {

class TaxiConfig;

class HttpClient final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "http-client";

  HttpClient(const ComponentConfig&, const ComponentContext&);

  ~HttpClient() override;

  clients::http::Client& GetHttpClient();

 private:
  template <typename ConfigTag>
  void OnConfigUpdate(
      const std::shared_ptr<taxi_config::BaseConfig<ConfigTag>>& config);

  formats::json::Value ExtendStatistics();

 private:
  std::shared_ptr<clients::http::Client> http_client_;
  components::TaxiConfig& taxi_config_component_;
  utils::AsyncEventSubscriberScope subscriber_scope_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components
