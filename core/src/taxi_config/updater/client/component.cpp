#include <taxi_config/updater/client/component.hpp>

#include <fstream>

#include <cache/update_type.hpp>
#include <clients/http/component.hpp>
#include <formats/json/serialize.hpp>
#include <fs/blocking/read.hpp>
#include <utils/string_to_duration.hpp>

namespace components {

TaxiConfigClientUpdater::TaxiConfigClientUpdater(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : CachingComponentBase(component_config, component_context, kName),
      taxi_config_(component_context.FindComponent<TaxiConfig>()) {
  load_only_my_values_ = component_config.ParseBool("load-only-my-values");
  store_enabled_ = component_config.ParseBool("store-enabled");

  clients::taxi_config::ClientConfig config;
  config.timeout =
      utils::StringToDuration(component_config.ParseString("http-timeout"));
  config.retries = component_config.ParseInt("http-retries");
  config.config_url = component_config.ParseString("config-url");

  auto fallback_config_contents = fs::blocking::ReadFileContents(
      component_config.ParseString("fallback-path"));
  try {
    fallback_config_.Parse(fallback_config_contents, false);
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Cannot load fallback taxi config: ") +
                             ex.what());
  }

  try {
    config_client_ = std::make_unique<clients::taxi_config::Client>(
        component_context.FindComponent<HttpClient>().GetHttpClient(), config);
    StartPeriodicUpdates();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Config client updater initialization failed: " << e.what();
    taxi_config_.NotifyLoadingFailed(e.what());
    /* Start PeriodicTask without the 1st update:
     * TaxiConfig has been initialized with
     * config cached in FS. Components loading will continue.
     */
    StartPeriodicUpdates(Flag::kNoFirstUpdate);
  }
}

TaxiConfigClientUpdater::~TaxiConfigClientUpdater() { StopPeriodicUpdates(); }

void TaxiConfigClientUpdater::StoreIfEnabled() {
  auto ptr = Get();
  if (store_enabled_) taxi_config_.SetConfig(ptr);

  docs_map_keys_ = ptr->GetRequestedNames();
}

std::vector<std::string> TaxiConfigClientUpdater::GetDocsMapKeys() const {
  if (load_only_my_values_)
    return docs_map_keys_;
  else
    return {};
}

void TaxiConfigClientUpdater::Update(
    cache::UpdateType update_type,
    const std::chrono::system_clock::time_point& /*last_update*/,
    const std::chrono::system_clock::time_point& /*now*/,
    cache::UpdateStatisticsScope& stats) {
  if (update_type == cache::UpdateType::kFull) {
    auto reply = config_client_->FetchDocsMap(boost::none, GetDocsMapKeys());
    auto& docs_map = reply.docs_map;

    stats.IncreaseDocumentsReadCount(docs_map.Size());

    /* Don't check for timestamp, accept any timestamp.
     * Otherwise we might end up with constantly failing to make full update
     * as every full update we get a bit outdated reply.
     */

    ::taxi_config::DocsMap combined(fallback_config_);
    combined.MergeFromOther(std::move(docs_map));

    auto size = combined.Size();
    Emplace(std::move(combined));
    StoreIfEnabled();

    stats.Finish(size);
    server_timestamp_ = reply.timestamp;
  } else {
    // kIncremental
    auto reply =
        config_client_->FetchDocsMap(server_timestamp_, GetDocsMapKeys());
    auto& docs_map = reply.docs_map;

    /* Timestamp can be compared lexicographically */
    if (reply.timestamp < server_timestamp_) {
      stats.FinishNoChanges();
      return;
    }

    if (reply.docs_map.Size() == 0) {
      stats.FinishNoChanges();
      return;
    }

    stats.IncreaseDocumentsReadCount(docs_map.Size());

    ::taxi_config::DocsMap combined = *Get();
    combined.MergeFromOther(std::move(docs_map));

    auto size = combined.Size();
    Emplace(std::move(combined));
    StoreIfEnabled();

    stats.Finish(size);
    server_timestamp_ = reply.timestamp;
  }
}

}  // namespace components
