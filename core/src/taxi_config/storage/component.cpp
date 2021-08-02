#include <userver/taxi_config/storage/component.hpp>

#include <userver/fs/read.hpp>
#include <userver/fs/write.hpp>

#include <fmt/format.h>

#include <userver/taxi_config/storage_mock.hpp>
#include <userver/taxi_config/updater/client/component.hpp>

namespace components {

constexpr std::chrono::seconds kWaitInterval(5);

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      cache_{taxi_config::impl::SnapshotData{
          std::vector<taxi_config::KeyValue>{}}},
      fs_cache_path_(config["fs-cache-path"].As<std::string>()),
      fs_task_processor_(
          fs_cache_path_.empty()
              ? nullptr
              : &context.GetTaskProcessor(
                    config["fs-task-processor"].As<std::string>())),
      is_loaded_(false),
      config_load_cancelled_(false),
      // If there is a known updater component we should not throw exceptions
      // on missing TaxiConfig::Updater instances as they will definitely
      // appear soon.
      updaters_count_{context.Contains(TaxiConfigClientUpdater::kName)} {
  ReadFsCache();
}

TaxiConfig::~TaxiConfig() = default;

taxi_config::Source TaxiConfig::GetSource() {
  WaitUntilLoaded();
  return taxi_config::Source{cache_};
}

void TaxiConfig::WaitUntilLoaded() {
  if (Has()) return;

  LOG_TRACE() << "Wait started";
  {
    std::unique_lock lock(loaded_mutex_);
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

void TaxiConfig::DoSetConfig(const taxi_config::DocsMap& value) {
  auto config = taxi_config::impl::SnapshotData(value, {});
  {
    std::lock_guard lock(loaded_mutex_);
    cache_.config.Assign(std::move(config));
    is_loaded_ = true;
  }
  loaded_cv_.NotifyAll();
  cache_.channel.SendEvent(taxi_config::Source{cache_}.GetSnapshot());
}

void TaxiConfig::SetConfig(const taxi_config::DocsMap& value) {
  DoSetConfig(value);
  WriteFsCache(value);
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

bool TaxiConfig::Has() const { return is_loaded_.load(); }

bool TaxiConfig::IsFsCacheEnabled() const {
  UASSERT_MSG(fs_cache_path_.empty() || fs_task_processor_,
              "fs_task_processor_ must be set if there is a fs_cache_path_");
  return !fs_cache_path_.empty();
}

void TaxiConfig::ReadFsCache() {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("taxi_config_fs_cache_read");
  try {
    if (!fs::FileExists(*fs_task_processor_, fs_cache_path_)) {
      LOG_WARNING() << "No FS cache for config found, waiting until "
                       "TaxiConfigClientUpdater fetches fresh configs";
      return;
    }
    const auto contents =
        fs::ReadFileContents(*fs_task_processor_, fs_cache_path_);

    taxi_config::DocsMap docs_map;
    docs_map.Parse(contents, false);

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

TaxiConfig::Updater::Updater(TaxiConfig& config_to_update) noexcept
    : config_to_update_(config_to_update) {
  ++config_to_update_.updaters_count_;
}

void TaxiConfig::Updater::SetConfig(const taxi_config::DocsMap& value_ptr) {
  config_to_update_.SetConfig(value_ptr);
}

void TaxiConfig::Updater::NotifyLoadingFailed(
    const std::string& updater_error) {
  config_to_update_.NotifyLoadingFailed(updater_error);
}

TaxiConfig::Updater::~Updater() { --config_to_update_.updaters_count_; }

TaxiConfig::NoblockSubscriber::NoblockSubscriber(
    TaxiConfig& config_component) noexcept
    : config_component_(config_component) {}

concurrent::AsyncEventChannel<const taxi_config::Snapshot&>&
TaxiConfig::NoblockSubscriber::GetEventChannel() noexcept {
  return config_component_.cache_.channel;
}

}  // namespace components
