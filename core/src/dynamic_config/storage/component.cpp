#include <userver/dynamic_config/storage/component.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_set>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/fs/read.hpp>
#include <userver/fs/write.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr std::chrono::seconds kWaitInterval(5);
}  // namespace

class DynamicConfig::Impl final {
 public:
  Impl(const ComponentConfig&, const ComponentContext&);

  dynamic_config::Source GetSource();
  auto& GetChannel() { return cache_.GetChannel(); }

  void OnLoadingCancelled();

  void SetConfig(std::string_view updater,
                 const dynamic_config::DocsMap& value);
  void NotifyLoadingFailed(std::string_view updater, std::string_view error);

 private:
  void DoSetConfig(const dynamic_config::DocsMap& value);

  bool Has() const;
  void WaitUntilLoaded();

  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const dynamic_config::DocsMap&);

  dynamic_config::impl::StorageData cache_;

  const std::string fs_cache_path_;
  engine::TaskProcessor* fs_task_processor_;
  std::string fs_loading_error_msg_;

  std::atomic<bool> is_loaded_{false};
  mutable engine::Mutex loaded_mutex_;
  mutable engine::ConditionVariable loaded_cv_;
  bool config_load_cancelled_{false};
};

DynamicConfig::Impl::Impl(const ComponentConfig& config,
                          const ComponentContext& context)
    : fs_cache_path_(config["fs-cache-path"].As<std::string>()),
      fs_task_processor_(
          fs_cache_path_.empty()
              ? nullptr
              : &context.GetTaskProcessor(
                    config["fs-task-processor"].As<std::string>())) {
  UINVARIANT(dynamic_config::impl::has_updater,
             fmt::format("At least one dynamic config updater component "
                         "responsible for DynamicConfig initialization should "
                         "be defined in a static config!"));
  ReadFsCache();
}

dynamic_config::Source DynamicConfig::Impl::GetSource() {
  WaitUntilLoaded();
  return dynamic_config::Source{cache_};
}

void DynamicConfig::Impl::WaitUntilLoaded() {
  if (Has()) return;

  LOG_TRACE() << "Wait started";
  {
    std::unique_lock lock(loaded_mutex_);
    while (!Has() && !config_load_cancelled_) {
      const auto res = loaded_cv_.WaitFor(lock, kWaitInterval);
      if (res == engine::CvStatus::kTimeout) {
        std::string_view fs_note = " Last error while reading config from FS: ";
        if (fs_loading_error_msg_.empty()) fs_note = {};
        LOG_WARNING() << "Waiting for the config load." << fs_note
                      << fs_loading_error_msg_;
      }
    }
  }
  LOG_TRACE() << "Wait finished";

  if (!Has() || config_load_cancelled_)
    throw ComponentsLoadCancelledException("config load cancelled");
}

void DynamicConfig::Impl::NotifyLoadingFailed(std::string_view updater,
                                              std::string_view error) {
  if (!Has()) {
    std::string message = "Failed to fetch initial dynamic config values. ";
    if (!fs_loading_error_msg_.empty()) {
      message += fmt::format(
          "We previously tried to load dynamic config from filesystem cache at "
          "'{}', which failed with a NON-FATAL error: ({}). We then tried to "
          "fetch up-to-date config values, which failed FATALLY as follows. ",
          fs_cache_path_, fs_loading_error_msg_);
    }
    message += fmt::format("Error from '{}' updater: ({})", updater, error);
    throw std::runtime_error(message);
  }
}

void DynamicConfig::Impl::DoSetConfig(const dynamic_config::DocsMap& value) {
  dynamic_config::impl::SnapshotData config(value, {});
  auto after_assign_hook = [&] {
    {
      std::lock_guard lock(loaded_mutex_);
      is_loaded_ = true;
    }
    loaded_cv_.NotifyAll();
  };
  cache_.Update(std::move(config), std::move(after_assign_hook));
}

void DynamicConfig::Impl::SetConfig(std::string_view updater,
                                    const dynamic_config::DocsMap& value) {
  LOG_DEBUG() << "Setting new dynamic config value from '" << updater << "'";
  DoSetConfig(value);
  WriteFsCache(value);
}

void DynamicConfig::Impl::OnLoadingCancelled() {
  if (Has()) return;

  LOG_WARNING() << "Components load was cancelled before DynamicConfig was "
                   "loaded. Please see previous logs for the failure reason";
  {
    std::lock_guard<engine::Mutex> lock(loaded_mutex_);
    config_load_cancelled_ = true;
  }
  loaded_cv_.NotifyAll();
}

bool DynamicConfig::Impl::Has() const { return is_loaded_.load(); }

bool DynamicConfig::Impl::IsFsCacheEnabled() const {
  UASSERT_MSG(fs_cache_path_.empty() || fs_task_processor_,
              "fs_task_processor_ must be set if there is a fs_cache_path_");
  return !fs_cache_path_.empty();
}

void DynamicConfig::Impl::ReadFsCache() {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("dynamic_config_fs_cache_read");
  try {
    if (!fs::FileExists(*fs_task_processor_, fs_cache_path_)) {
      fs_loading_error_msg_ = "No cache file found";
      LOG_WARNING() << "No filesystem cache for dynamic config found, waiting "
                       "until the updater fetches fresh configs";
      return;
    }
    const auto contents =
        fs::ReadFileContents(*fs_task_processor_, fs_cache_path_);

    dynamic_config::DocsMap docs_map;
    docs_map.Parse(contents, false);

    DoSetConfig(docs_map);
    LOG_INFO() << "Successfully read dynamic_config from FS cache";
  } catch (const std::exception& e) {
    /* Possible sources of error:
     * 1) cache file suddenly disappeared
     * 2) cache file is broken
     * 3) cache file is created by an old server version, it misses some
     * variables which are needed in current server version
     */
    fs_loading_error_msg_ =
        fmt::format("{} ({})", e.what(), compiler::GetTypeName(typeid(e)));
    LOG_WARNING() << "Failed to load dynamic config from filesystem cache at '"
                  << fs_cache_path_ << "': " << e;
  }
}

void DynamicConfig::Impl::WriteFsCache(
    const dynamic_config::DocsMap& docs_map) {
  if (!IsFsCacheEnabled()) return;

  tracing::Span span("dynamic_config_fs_cache_write");
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

    LOG_INFO() << "Successfully wrote dynamic_config from FS cache";
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to save config to FS cache '" << fs_cache_path_
                << "': " << e;
  }
}

DynamicConfig::NoblockSubscriber::NoblockSubscriber(
    DynamicConfig& config_component) noexcept
    : config_component_(config_component) {}

concurrent::AsyncEventSource<const dynamic_config::Snapshot&>&
DynamicConfig::NoblockSubscriber::GetEventSource() noexcept {
  return config_component_.impl_->GetChannel();
}

DynamicConfig::DynamicConfig(const ComponentConfig& config,
                             const ComponentContext& context)
    : DynamicConfigUpdatesSinkBase(config, context),
      impl_(std::make_unique<Impl>(config, context)) {}

DynamicConfig::~DynamicConfig() = default;

dynamic_config::Source DynamicConfig::GetSource() { return impl_->GetSource(); }

void DynamicConfig::OnLoadingCancelled() { impl_->OnLoadingCancelled(); }

void DynamicConfig::SetConfig(std::string_view updater,
                              dynamic_config::DocsMap&& value) {
  impl_->SetConfig(updater, value);
}

void DynamicConfig::SetConfig(std::string_view updater,
                              const dynamic_config::DocsMap& value) {
  impl_->SetConfig(updater, value);
}

void DynamicConfig::NotifyLoadingFailed(std::string_view updater,
                                        std::string_view error) {
  impl_->NotifyLoadingFailed(updater, error);
}

yaml_config::Schema DynamicConfig::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that stores the runtime config.
additionalProperties: false
properties:
    fs-cache-path:
        type: string
        description: path to the file to read and dump a config cache; set to empty string to disable reading and dumping configs to FS
    fs-task-processor:
        type: string
        description: name of the task processor to run the blocking file write operations
)");
}

}  // namespace components

USERVER_NAMESPACE_END
