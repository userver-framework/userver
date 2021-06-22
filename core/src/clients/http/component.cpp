#include <clients/http/component.hpp>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/statistics_storage.hpp>
#include <taxi_config/storage/component.hpp>
#include <utils/statistics/metadata.hpp>

#include <clients/http/client.hpp>
#include <clients/http/config.hpp>
#include <clients/http/destination_statistics_json.hpp>
#include <clients/http/statistics.hpp>
#include <clients/http/testsuite.hpp>

namespace components {

namespace {
constexpr size_t kHttpClientThreadsDefault = 8;
constexpr size_t kDestinationMetricsAutoMaxSizeDefault = 100;
}  // namespace

HttpClient::HttpClient(const ComponentConfig& component_config,
                       const ComponentContext& context)
    : LoggableComponentBase(component_config, context),
      disable_pool_stats_(
          component_config["pool-statistics-disable"].As<bool>(false)),
      http_client_(
          component_config["thread-name-prefix"].As<std::string>(""),
          component_config["threads"].As<size_t>(kHttpClientThreadsDefault),
          context.GetTaskProcessor(
              component_config["fs-task-processor"].As<std::string>())) {
  http_client_.SetDestinationMetricsAutoMaxSize(
      component_config["destination-metrics-auto-max-size"].As<size_t>(
          kDestinationMetricsAutoMaxSizeDefault));

  auto user_agent =
      component_config["user-agent"].As<std::optional<std::string>>();
  if (user_agent) {
    if (!user_agent->empty()) {
      http_client_.ResetUserAgent(std::move(*user_agent));
    } else {
      http_client_.ResetUserAgent({});  // no user agent
    }
  } else {
    // Leaving the default one
  }

  auto testsuite_enabled =
      component_config["testsuite-enabled"].As<bool>(false);
  if (testsuite_enabled) {
    const auto& timeout = component_config["testsuite-timeout"]
                              .As<std::optional<std::chrono::milliseconds>>();
    auto prefixes = component_config["testsuite-allowed-url-prefixes"]
                        .As<std::vector<std::string>>({});
    http_client_.SetTestsuiteConfig({prefixes, timeout});
  }

  clients::http::Config bootstrap_config;
  bootstrap_config.proxy =
      component_config["bootstrap-http-proxy"].As<std::string>({});
  http_client_.SetConfig(bootstrap_config);

  auto& taxi_config = context.FindComponent<components::TaxiConfig>();
  subscriber_scope_ = taxi_config.GetEventChannel().AddListener(
      this, "http_client", &HttpClient::OnConfigUpdate);

  const auto thread_name_prefix =
      component_config["thread-name-prefix"].As<std::string>("");
  auto stats_name =
      "httpclient" +
      (thread_name_prefix.empty() ? "" : ("-" + thread_name_prefix));
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      stats_name,
      [this](const utils::statistics::StatisticsRequest& /*request*/) {
        return ExtendStatistics();
      });
}

clients::http::Client& HttpClient::GetHttpClient() { return http_client_; }

void HttpClient::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::Snapshot>& config) {
  http_client_.SetConfig(config->Get<clients::http::Config>());
}

formats::json::Value HttpClient::ExtendStatistics() {
  formats::json::ValueBuilder json;
  if (!disable_pool_stats_) {
    json =
        clients::http::PoolStatisticsToJson(http_client_.GetPoolStatistics());
  }
  json["destinations"] = clients::http::DestinationStatisticsToJson(
      http_client_.GetDestinationStatistics());
  utils::statistics::SolomonChildrenAreLabelValues(json["destinations"],
                                                   "http_destination");
  utils::statistics::SolomonSkip(json["destinations"]);
  return json.ExtractValue();
}

}  // namespace components
