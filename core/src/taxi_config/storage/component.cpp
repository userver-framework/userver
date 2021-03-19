#include <taxi_config/storage/component.hpp>

#include <taxi_config/updater/client/component.hpp>

#include <fs/read.hpp>
#include <fs/write.hpp>

#include <fmt/format.h>

namespace components {

constexpr std::chrono::seconds kWaitInterval(5);

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      utils::AsyncEventChannel<
          const std::shared_ptr<const taxi_config::Config>&>(kName),
      fs_cache_path_(config["fs-cache-path"].As<std::string>()),
      fs_task_processor_(
          fs_cache_path_.empty()
              ? nullptr
              : &context.GetTaskProcessor(
                    config["fs-task-processor"].As<std::string>())),
      config_load_cancelled_(false),
      // If there is a known updater component we should not throw exceptions
      // on missing TaxiConfig::Updater instances as they will definitely
      // appear soon.
      updaters_count_{context.Contains(TaxiConfigClientUpdater::kName)} {
  ReadBootstrap(config["bootstrap-path"].As<std::string>());
  ReadFsCache();
}

TaxiConfig::~TaxiConfig() = default;

std::shared_ptr<const taxi_config::Config> TaxiConfig::Get() const {
  auto ptr = cache_ptr_.ReadCopy();
  if (ptr) return ptr;

  LOG_TRACE() << "Wait started";
  {
    std::unique_lock<engine::Mutex> lock(loaded_mutex_);
    while (!Has() && !config_load_cancelled_) {
      const auto res = loaded_cv_.WaitFor(lock, kWaitInterval);
      if (res == engine::CvStatus::kTimeout) {
        std::string message;
        if (!fs_loading_error_msg_.empty()) {
          message = " Last error while reading config from FS: " +
                    fs_loading_error_msg_;
        }

        if (updaters_count_.load() == 0) {
          throw std::runtime_error(
              "There is no instance of a TaxiConfig::Updater so far. "
              "No one is able to update the uninitialized TaxiConfig. "
              "Make sure that the config updater "
              "component (for example components::TaxiConfigClientUpdater) "
              "is registered in the component list or that the "
              "`taxi-config.fs-cache-path` is "
              "set to a proper path in static config of the service." +
              message);
        }

        LOG_WARNING() << "Waiting for the config load." << message;
      }
    }
  }
  LOG_TRACE() << "Wait finished";

  if (!Has() || config_load_cancelled_)
    throw ComponentsLoadCancelledException("config load cancelled");
  return cache_ptr_.ReadCopy();
}

void TaxiConfig::NotifyLoadingFailed(const std::string& updater_error) {
  if (!Has()) {
    std::string message;
    if (!fs_loading_error_msg_.empty()) {
      message += "TaxiConfig error: " + fs_loading_error_msg_ + ".\n";
    }
    message += "Error from updater: " + updater_error;
    throw std::runtime_error(message);
  }
}

std::shared_ptr<const taxi_config::BootstrapConfig> TaxiConfig::GetBootstrap()
    const {
  return bootstrap_config_;
}

std::shared_ptr<const taxi_config::Config> TaxiConfig::GetNoblock() const {
  return cache_ptr_.ReadCopy();
}

void TaxiConfig::DoSetConfig(
    const std::shared_ptr<const taxi_config::DocsMap>& value_ptr) {
  auto config = taxi_config::Config::Parse(*value_ptr);
  {
    std::lock_guard<engine::Mutex> lock(loaded_mutex_);
    if (cache_) {
      cache_->Assign(std::move(config));
    } else {
      cache_.emplace(std::move(config));
    }
    auto cache_snapshot =
        std::make_shared<rcu::ReadablePtr<taxi_config::Config>>(cache_->Read());
    const auto& cache_ref = **cache_snapshot;
    cache_ptr_.Assign(std::shared_ptr<const taxi_config::Config>{
        // NOLINTNEXTLINE(hicpp-move-const-arg)
        std::move(cache_snapshot), &cache_ref});
  }
  loaded_cv_.NotifyAll();
  SendEvent(cache_ptr_.ReadCopy());
}

void TaxiConfig::SetConfig(
    std::shared_ptr<const taxi_config::DocsMap> value_ptr) {
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

bool TaxiConfig::Has() const {
  const auto ptr = cache_ptr_.Read();
  return static_cast<bool>(*ptr);
}

bool TaxiConfig::IsFsCacheEnabled() const {
  UASSERT_MSG(fs_cache_path_.empty() || fs_task_processor_,
              "fs_task_processor_ must be set if there is a fs_cache_path_");
  return !fs_cache_path_.empty();
}

void TaxiConfig::ReadFsCache() {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("taxi_config_fs_cache_read");
  try {
    auto docs_map = std::make_shared<::taxi_config::DocsMap>();

    if (!fs::FileExists(*fs_task_processor_, fs_cache_path_)) {
      LOG_WARNING() << "No FS cache for config found, waiting until "
                       "TaxiConfigClientUpdater fetches fresh configs";
      return;
    }

    const auto contents =
        fs::ReadFileContents(*fs_task_processor_, fs_cache_path_);
    docs_map->Parse(contents, false);

    DoSetConfig(docs_map);
    LOG_INFO() << "Successfully read taxi_config from FS cache";
  } catch (const std::exception& e) {
    /* Possible sources of error:
     * 1) cache file suddenly disappeared
     * 2) cache file is broken
     * 3) cache file is created by an old server version, it misses some
     * variables which are needed in current server version
     */
    fs_loading_error_msg_ = fmt::format(
        "Failed to load config from FS cache '{}'. ", fs_cache_path_);
    LOG_WARNING() << fs_loading_error_msg_ << e;
    fs_loading_error_msg_ += e.what();
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
    fs::CreateDirectories(
        *fs_task_processor_,
        boost::filesystem::path(fs_cache_path_).parent_path().string());
    fs::RewriteFileContentsAtomically(*fs_task_processor_, fs_cache_path_,
                                      contents, mode);

    LOG_INFO() << "Successfully wrote taxi_config from FS cache";
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to save config to FS cache '" << fs_cache_path_
                << "': " << e;
  }
}

void TaxiConfig::ReadBootstrap(const std::string& bootstrap_fname) {
  try {
    auto bootstrap_config_contents =
        fs::ReadFileContents(*fs_task_processor_, bootstrap_fname);
    taxi_config::DocsMap bootstrap_config;
    bootstrap_config.Parse(bootstrap_config_contents, false);
    bootstrap_config_ = std::make_shared<taxi_config::BootstrapConfig>(
        taxi_config::BootstrapConfig::Parse(bootstrap_config));
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        fmt::format("Cannot load bootstrap taxi config '{}'. {}",
                    bootstrap_fname, ex.what()));
  }
}

}  // namespace components
