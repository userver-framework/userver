#include <boost/algorithm/string.hpp>

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
              component_config["fs-task-processor"].As<std::string>())),
      taxi_config_component_(context.FindComponent<components::TaxiConfig>()) {
  http_client_.SetDestinationMetricsAutoMaxSize(
      component_config["destination-metrics-auto-max-size"].As<size_t>(
          kDestinationMetricsAutoMaxSizeDefault));

  auto testsuite_enabled =
      component_config["testsuite-enabled"].As<bool>(false);
  if (testsuite_enabled) {
    const auto& timeout = component_config["testsuite-timeout"]
                              .As<std::optional<std::chrono::milliseconds>>();
    auto prefixes_lines =
        component_config["testsuite-allowed-url-prefixes"].As<std::string>("");
    std::vector<std::string> prefixes;
    // TODO replace splitting string by config.Parse<std::vector<std::string>>
    // as soon as https://st.yandex-team.ru/TAXICOMMON-1599 gets fixed
    boost::split(prefixes, prefixes_lines, boost::is_any_of(" \t\r\n"));
    http_client_.SetTestsuiteConfig({prefixes, timeout});
  }

  subscriber_scope_ = taxi_config_component_.AddListener(
      this, "http_client",
      &HttpClient::OnConfigUpdate<taxi_config::FullConfigTag>);

  // Use up-to-date config if available, a bootstrap config otherwise
  // The next OnConfigUpdate() will read taxi_config::Config with a delay anyway
  auto taxi_config = taxi_config_component_.GetNoblock();
  if (taxi_config)
    OnConfigUpdate(taxi_config);
  else
    OnConfigUpdate(taxi_config_component_.GetBootstrap());

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

template <typename ConfigTag>
void HttpClient::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::BaseConfig<ConfigTag>>& config) {
  http_client_.SetConfig(config->template Get<clients::http::Config>());
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
