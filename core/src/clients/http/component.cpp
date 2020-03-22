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
const auto kDestinationMetricsAutoMaxSizeDefault = 100;
}  // namespace

HttpClient::HttpClient(const ComponentConfig& component_config,
                       const ComponentContext& context)
    : LoggableComponentBase(component_config, context),
      taxi_config_component_(context.FindComponent<components::TaxiConfig>()) {
  auto booststrap_config = taxi_config_component_.GetBootstrap();
  const auto& http_config = booststrap_config->Get<clients::http::Config>();
  size_t threads = http_config.threads;

  auto thread_name_prefix =
      component_config.ParseString("thread-name-prefix", "");
  http_client_ = clients::http::Client::Create(thread_name_prefix, threads);
  http_client_->SetDestinationMetricsAutoMaxSize(
      component_config.ParseInt("destination-metrics-auto-max-size",
                                kDestinationMetricsAutoMaxSizeDefault));

  auto testsuite_enabled =
      component_config.ParseBool("testsuite-enabled", false);
  if (testsuite_enabled) {
    const auto& timeout =
        component_config.ParseOptionalDuration("testsuite-timeout");
    auto prefixes_lines =
        component_config.ParseString("testsuite-allowed-url-prefixes", "");
    std::vector<std::string> prefixes;
    // TODO replace splitting string by config.Parse<std::vector<std::string>>
    // as soon as https://st.yandex-team.ru/TAXICOMMON-1599 gets fixed
    boost::split(prefixes, prefixes_lines, boost::is_any_of(" \t\r\n"));
    http_client_->SetTestsuiteConfig({prefixes, timeout});
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
    OnConfigUpdate(booststrap_config);

  try {
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
  } catch (...) {
    subscriber_scope_.Unsubscribe();
    throw;
  }
}

HttpClient::~HttpClient() {
  statistics_holder_.Unregister();
  subscriber_scope_.Unsubscribe();
}

clients::http::Client& HttpClient::GetHttpClient() {
  if (!http_client_) {
    LOG_ERROR() << "Asking for http client after components::HttpClient "
                   "destructor is called.";
    logging::LogFlush();
    abort();
  }
  return *http_client_;
}

template <typename ConfigTag>
void HttpClient::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::BaseConfig<ConfigTag>>& config) {
  const auto& http_client_config =
      config->template Get<clients::http::Config>();
  http_client_->SetConnectionPoolSize(http_client_config.connection_pool_size);

  http_client_->SetConnectRatelimitHttp(
      http_client_config.http_connect_throttle_max_size_,
      http_client_config.http_connect_throttle_update_interval_);
  http_client_->SetConnectRatelimitHttps(
      http_client_config.https_connect_throttle_max_size_,
      http_client_config.https_connect_throttle_update_interval_);
}

formats::json::Value HttpClient::ExtendStatistics() {
  auto json =
      clients::http::PoolStatisticsToJson(http_client_->GetPoolStatistics());
  json["destinations"] = clients::http::DestinationStatisticsToJson(
      http_client_->GetDestinationStatistics());
  utils::statistics::SolomonChildrenAreLabelValues(json["destinations"],
                                                   "http_destination");
  utils::statistics::SolomonSkip(json["destinations"]);
  return json.ExtractValue();
}

}  // namespace components
