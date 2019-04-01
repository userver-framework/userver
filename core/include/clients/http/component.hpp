#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <taxi_config/storage/component.hpp>

#include <components/statistics_storage.hpp>
#include <utils/async_event_channel.hpp>

namespace clients {
namespace http {
class Client;
}
}  // namespace clients

namespace components {

class HttpClient : public LoggableComponentBase {
 public:
  static constexpr auto kName = "http-client";

  HttpClient(const ComponentConfig&, const ComponentContext&);

  ~HttpClient();

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
