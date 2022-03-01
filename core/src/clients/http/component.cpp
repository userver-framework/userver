#include <userver/clients/http/component.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/utils/statistics/metadata.hpp>

#include <clients/http/destination_statistics_json.hpp>
#include <clients/http/testsuite.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/config.hpp>
#include <userver/clients/http/statistics.hpp>

USERVER_NAMESPACE_BEGIN

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

  http_client_.SetDnsResolver(
      clients::dns::GetResolverPtr(component_config, context));

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

  auto& config_component = context.FindComponent<components::TaxiConfig>();
  subscriber_scope_ =
      components::TaxiConfig::NoblockSubscriber{config_component}
          .GetEventSource()
          .AddListener(this, kName, &HttpClient::OnConfigUpdate);

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

HttpClient::~HttpClient() {
  subscriber_scope_.Unsubscribe();
  statistics_holder_.Unregister();
}

clients::http::Client& HttpClient::GetHttpClient() { return http_client_; }

void HttpClient::OnConfigUpdate(const dynamic_config::Snapshot& config) {
  http_client_.SetConfig(config.Get<clients::http::Config>());
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

USERVER_NAMESPACE_END
