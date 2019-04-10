#include <taxi_config/storage/component.hpp>

#include <fstream>

#include <fs/read.hpp>
#include <fs/write.hpp>

namespace components {

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      utils::AsyncEventChannel<const std::shared_ptr<taxi_config::Config>&>(
          kName),
      config_load_cancelled_(false),
      fs_task_processor_(context.GetTaskProcessor(
          config.ParseString("fs-task-processor-name"))),
      fs_cache_path_(config.ParseString("fs-cache-path")) {
  ReadBootstrap(config.ParseString("bootstrap-path"));

  ReadFsCache();
}

TaxiConfig::~TaxiConfig() = default;

std::shared_ptr<taxi_config::Config> TaxiConfig::Get() const {
  auto ptr = cache_.Get();
  if (ptr) return ptr;

  LOG_TRACE() << "Wait started";
  bool was_loaded;
  {
    std::unique_lock<engine::Mutex> lock(loaded_mutex_);
    was_loaded = loaded_cv_.Wait(lock, [this]() {
      return cache_.Get() != nullptr || config_load_cancelled_;
    });
  }
  LOG_TRACE() << "Wait finished";

  if (!was_loaded || config_load_cancelled_)
    throw ComponentsLoadCancelledException("config load cancelled");
  return cache_.Get();
}

void TaxiConfig::NotifyLoadingFailed(const std::string& updater_error) {
  if (!Has()) {
    std::string message;
    if (!loading_error_msg_.empty()) {
      message += "TaxiConfig error: " + loading_error_msg_ + ", ";
    }
    message += "error from updater: " + updater_error;
    throw std::runtime_error(message);
  }
}

std::shared_ptr<taxi_config::BootstrapConfig> TaxiConfig::GetBootstrap() const {
  return bootstrap_config_;
}

void TaxiConfig::DoSetConfig(
    const std::shared_ptr<taxi_config::DocsMap>& value_ptr) {
  auto config = std::make_shared<taxi_config::Config>(*value_ptr);
  {
    std::lock_guard<engine::Mutex> lock(loaded_mutex_);
    cache_.Set(config);
  }
  loaded_cv_.NotifyAll();
  SendEvent(cache_.Get());
}

void TaxiConfig::SetConfig(std::shared_ptr<taxi_config::DocsMap> value_ptr) {
  DoSetConfig(value_ptr);
  WriteFsCache(*value_ptr);
}

void TaxiConfig::OnLoadingCancelled() {
  if (Has()) return;

  LOG_ERROR() << "Config was set to nullptr, config load cancelled";
  {
    std::lock_guard<engine::Mutex> lock(loaded_mutex_);
    config_load_cancelled_ = true;
  }
  loaded_cv_.NotifyAll();
}

bool TaxiConfig::Has() const { return !!cache_.Get(); }

bool TaxiConfig::IsFsCacheEnabled() const { return !fs_cache_path_.empty(); }

void TaxiConfig::ReadFsCache() {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("taxi_config_fs_cache_read");
  try {
    auto docs_map = std::make_shared<::taxi_config::DocsMap>();

    const auto contents =
        fs::ReadFileContents(fs_task_processor_, fs_cache_path_);
    docs_map->Parse(contents, false);

    DoSetConfig(docs_map);
    LOG_INFO() << "Successfully read taxi_config from FS cache";
  } catch (const std::exception& e) {
    /* Possible sources of error:
     * 1) no cache
     * 2) cache file is broken
     * 3) cache file is created by an old server version, it misses some
     * variables which are needed in current server version
     */
    loading_error_msg_ =
        std::string("Failed to load config from FS cache: ") + e.what();
    LOG_WARNING() << loading_error_msg_;
  }
}

void TaxiConfig::WriteFsCache(const taxi_config::DocsMap& docs_map) {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("taxi_config_fs_cache_write");
  try {
    const auto contents = docs_map.AsJsonString();
    using perms = boost::filesystem::perms;
    auto mode = perms::owner_read | perms::owner_write | perms::group_read |
                perms::others_read;
    fs::RewriteFileContentsAtomically(fs_task_processor_, fs_cache_path_,
                                      contents, mode);

    LOG_INFO() << "Successfully wrote taxi_config from FS cache";
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to save config to FS cache: " << e;
  }
}

void TaxiConfig::ReadBootstrap(const std::string& bootstrap_fname) {
  try {
    auto bootstrap_config_contents =
        fs::ReadFileContents(fs_task_processor_, bootstrap_fname);
    taxi_config::DocsMap bootstrap_config;
    bootstrap_config.Parse(bootstrap_config_contents, false);
    bootstrap_config_ =
        std::make_shared<taxi_config::BootstrapConfig>(bootstrap_config);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        std::string("Cannot load bootstrap taxi config: ") + ex.what());
  }
}

}  // namespace components
