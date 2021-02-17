#pragma once

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use components::Http from clients/http.hpp instead
#endif

#include <clients/http/client.hpp>
#include <components/loggable_component_base.hpp>
#include <components/statistics_storage.hpp>
#include <taxi_config/config.hpp>
#include <utils/async_event_channel.hpp>

namespace components {

class TaxiConfig;

/// @ingroup userver_components
///
class HttpClient final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "http-client";

  HttpClient(const ComponentConfig&, const ComponentContext&);

  clients::http::Client& GetHttpClient();

 private:
  template <typename ConfigTag>
  void OnConfigUpdate(
      const std::shared_ptr<const taxi_config::BaseConfig<ConfigTag>>& config);

  formats::json::Value ExtendStatistics();

 private:
  const bool disable_pool_stats_;
  clients::http::Client http_client_;
  components::TaxiConfig& taxi_config_component_;
  utils::AsyncEventSubscriberScope subscriber_scope_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components
