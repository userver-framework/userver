#include <userver/dynamic_config/storage/component.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_set>

#include <fmt/format.h>

#include <boost/filesystem/operations.hpp>

#include <userver/alerts/component.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/exception.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/fs/read.hpp>
#include <userver/fs/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr std::chrono::seconds kWaitInterval(5);

struct DynamicConfigStatistics final {
    std::atomic<bool> was_last_parse_successful{true};
    utils::statistics::RateCounter parse_errors;
};

bool AreCacheDumpsEnabled(const components::ComponentContext& context) {
    auto* const testsuite_support = context.FindComponentOptional<components::TestsuiteSupport>();
    if (!testsuite_support) return true;
    return testsuite_support->GetDumpControl().GetPeriodicsMode() == testsuite::DumpControl::PeriodicsMode::kEnabled;
}

}  // namespace

class DynamicConfig::Impl final {
public:
    Impl(const ComponentConfig&, const ComponentContext&);

    dynamic_config::Source GetSource();
    auto& GetChannel() { return cache_.GetChannel(); }

    const dynamic_config::DocsMap& GetDefaultDocsMap() const;
    bool AreUpdatesEnabled() const;

    ComponentHealth GetComponentHealth() const;
    void OnLoadingCancelled();

    void SetConfig(std::string_view updater, dynamic_config::DocsMap&& value);
    void NotifyLoadingFailed(std::string_view updater, std::string_view error);

private:
    dynamic_config::impl::SnapshotData ParseConfig(const dynamic_config::DocsMap& value);

    void DoSetConfig(const dynamic_config::DocsMap& value);

    bool Has() const;
    void WaitUntilLoaded();

    void ReadFallback(const ComponentConfig& config);

    void ReadFsCache();
    void WriteFsCache(const dynamic_config::DocsMap&);

    void WriteStatistics(utils::statistics::Writer& writer) const;

    alerts::Storage& alert_storage_;
    const std::string fs_cache_path_;
    engine::TaskProcessor* fs_task_processor_;

    dynamic_config::impl::StorageData cache_;
    std::string fs_loading_error_msg_;
    dynamic_config::DocsMap fallback_config_;

    const bool updates_enabled_;
    const bool fs_write_enabled_;
    std::atomic<bool> is_loaded_{false};
    bool config_load_cancelled_{false};
    std::optional<std::string> load_cancellation_message_;
    mutable engine::Mutex loaded_mutex_;
    mutable engine::ConditionVariable loaded_cv_;
    DynamicConfigStatistics stats_;

    // Must be the last field
    utils::statistics::Entry statistics_holder_;
};

DynamicConfig::Impl::Impl(const ComponentConfig& config, const ComponentContext& context)
    : alert_storage_(context.FindComponent<alerts::StorageComponent>().GetStorage()),
      fs_cache_path_(config["fs-cache-path"].As<std::string>({})),
      fs_task_processor_([&] {
          const auto name = config["fs-task-processor"].As<std::optional<std::string>>();
          return name ? &context.GetTaskProcessor(*name) : nullptr;
      }()),
      updates_enabled_(config["updates-enabled"].As<bool>(false)),
      fs_write_enabled_(AreCacheDumpsEnabled(context)) {
    if (!fs_cache_path_.empty() && !fs_task_processor_) {
        throw std::logic_error("fs-task-processor must be set if there is fs-cache-path");
    }

    ReadFallback(config);
    ReadFsCache();

    statistics_holder_ = context.FindComponent<components::StatisticsStorage>().GetStorage().RegisterWriter(
        "dynamic-config", [this](auto& writer) { WriteStatistics(writer); }
    );
}

dynamic_config::Source DynamicConfig::Impl::GetSource() {
    WaitUntilLoaded();
    return dynamic_config::Source{cache_};
}

const dynamic_config::DocsMap& DynamicConfig::Impl::GetDefaultDocsMap() const { return fallback_config_; }

bool DynamicConfig::Impl::AreUpdatesEnabled() const { return updates_enabled_; }

void DynamicConfig::Impl::WaitUntilLoaded() {
    if (Has()) return;

    LOG_TRACE() << "Wait started";
    std::unique_lock lock(loaded_mutex_);
    while (!Has() && !config_load_cancelled_) {
        const auto res = loaded_cv_.WaitFor(lock, kWaitInterval);
        if (res == engine::CvStatus::kTimeout) {
            std::string_view fs_note = " Last error while reading config from FS: ";
            if (fs_loading_error_msg_.empty()) fs_note = {};
            LOG_WARNING() << "Waiting for the config load." << fs_note << fs_loading_error_msg_;
        }
    }
    LOG_TRACE() << "Wait finished";

    if (!Has() || config_load_cancelled_) {
        throw ComponentsLoadCancelledException(load_cancellation_message_.value_or("config load cancelled"));
    }
}

void DynamicConfig::Impl::NotifyLoadingFailed(std::string_view updater, std::string_view error) {
    if (!Has()) {
        std::string message = "Failed to fetch initial dynamic config values. ";
        if (!fs_loading_error_msg_.empty()) {
            message += fmt::format(
                "We previously tried to load dynamic config from filesystem cache at "
                "'{}', which failed with a NON-FATAL error: ({}). We then tried to "
                "fetch up-to-date config values, which failed FATALLY as follows. ",
                fs_cache_path_,
                fs_loading_error_msg_
            );
        }
        message += fmt::format("Error from '{}' updater: ({})", updater, error);

        {
            const std::lock_guard lock(loaded_mutex_);
            config_load_cancelled_ = true;
            load_cancellation_message_ = std::move(message);
        }
        loaded_cv_.NotifyAll();
    }
}

dynamic_config::impl::SnapshotData DynamicConfig::Impl::ParseConfig(const dynamic_config::DocsMap& value) {
    try {
        dynamic_config::impl::SnapshotData config(value, {});
        stats_.was_last_parse_successful = true;
        alert_storage_.StopAlertNow("config_parse_error");
        return config;
    } catch (const dynamic_config::ConfigParseError& e) {
        stats_.was_last_parse_successful = false;
        ++stats_.parse_errors;
        alert_storage_.FireAlert(
            "config_parse_error", std::string("Failed to parse dynamic config. ") + e.what(), alerts::kInfinity
        );
        throw;
    }
}

void DynamicConfig::Impl::DoSetConfig(const dynamic_config::DocsMap& value) {
    auto config = ParseConfig(value);

    if (!value.GetConfigsExpectedToBeUsed(utils::impl::InternalTag{}).empty()) {
        LOG_INFO() << "Some configs expected to be used are actually not needed: "
                   << value.GetConfigsExpectedToBeUsed(utils::impl::InternalTag{});
    }

    auto after_assign_hook = [&] {
        {
            const std::lock_guard lock(loaded_mutex_);
            is_loaded_ = true;
        }
        loaded_cv_.NotifyAll();
    };
    cache_.Update(std::move(config), std::move(after_assign_hook));
}

void DynamicConfig::Impl::SetConfig(std::string_view updater, dynamic_config::DocsMap&& value) {
    LOG_DEBUG() << "Setting new dynamic config value from '" << updater << "'";
    value.MergeMissing(fallback_config_);
    DoSetConfig(value);
    WriteFsCache(value);
}

ComponentHealth DynamicConfig::Impl::GetComponentHealth() const {
    return Has() ? ComponentHealth::kOk : ComponentHealth::kFatal;
}

void DynamicConfig::Impl::OnLoadingCancelled() {
    if (Has()) return;

    LOG_WARNING() << "Components load was cancelled before DynamicConfig was "
                     "loaded. Please see previous logs for the failure reason";
    {
        const std::lock_guard lock(loaded_mutex_);
        config_load_cancelled_ = true;
    }
    loaded_cv_.NotifyAll();
}

bool DynamicConfig::Impl::Has() const { return is_loaded_.load(); }

void DynamicConfig::Impl::ReadFallback(const ComponentConfig& config) {
    fallback_config_ = dynamic_config::impl::MakeDefaultDocsMap();

    const auto default_overrides_path = config["defaults-path"].As<std::optional<std::string>>();

    const auto default_overrides = config["defaults"].As<std::optional<formats::json::Value>>();

    if (default_overrides_path && default_overrides) {
        throw std::runtime_error(
            "Static config options dynamic-config.defaults and "
            "dynamic-config.defaults-path are incompatible"
        );
    }

    if (default_overrides_path) {
        if (!fs_task_processor_) {
            throw std::runtime_error(
                "dynamic-config.defaults-path option requires specifying "
                "dynamic-config.fs-task-processor"
            );
        }

        const tracing::Span span("dynamic_config_fallback_read");
        try {
            const auto fallback_contents = fs::ReadFileContents(*fs_task_processor_, *default_overrides_path);
            fallback_config_.Parse(fallback_contents, true);
        } catch (const std::exception& ex) {
            throw std::runtime_error(
                fmt::format("Failed to load dynamic config fallback from '{}': {}", *default_overrides_path, ex.what())
            );
        }
    }

    if (default_overrides) {
        for (const auto& [key, value] : Items(*default_overrides)) {
            fallback_config_.Set(key, value);
        }
    }
}

void DynamicConfig::Impl::ReadFsCache() {
    if (fs_cache_path_.empty()) return;

    const tracing::Span span("dynamic_config_fs_cache_read");
    try {
        if (!fs::FileExists(*fs_task_processor_, fs_cache_path_)) {
            fs_loading_error_msg_ = "No cache file found";
            LOG_WARNING() << "No filesystem cache for dynamic config found, waiting "
                             "until the updater fetches fresh configs";
            return;
        }
        const auto contents = fs::ReadFileContents(*fs_task_processor_, fs_cache_path_);

        dynamic_config::DocsMap docs_map;
        docs_map.Parse(contents, /*empty_ok=*/true);

        docs_map.MergeMissing(fallback_config_);
        DoSetConfig(docs_map);
        LOG_INFO() << "Successfully read dynamic_config from FS cache";
    } catch (const std::exception& e) {
        /* Possible sources of error:
         * 1) cache file suddenly disappeared
         * 2) cache file is broken
         * 3) cache file is created by an old server version, it misses some
         * variables which are needed in current server version
         */
        fs_loading_error_msg_ = fmt::format("{} ({})", e.what(), compiler::GetTypeName(typeid(e)));
        LOG_WARNING() << "Failed to load dynamic config from filesystem cache at '" << fs_cache_path_ << "': " << e;
    }
}

void DynamicConfig::Impl::WriteFsCache(const dynamic_config::DocsMap& docs_map) {
    if (fs_cache_path_.empty() || !fs_write_enabled_) return;

    const tracing::Span span("dynamic_config_fs_cache_write");
    try {
        const auto contents = formats::json::ToString(docs_map.AsJson());
        using perms = boost::filesystem::perms;
        auto mode = perms::owner_read | perms::owner_write | perms::group_read | perms::others_read;
        fs::CreateDirectories(*fs_task_processor_, boost::filesystem::path(fs_cache_path_).parent_path().string());
        fs::RewriteFileContentsAtomically(*fs_task_processor_, fs_cache_path_, contents, mode);

        LOG_INFO() << "Successfully wrote dynamic_config from FS cache";
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to save config to FS cache '" << fs_cache_path_ << "': " << e;
    }
}

void DynamicConfig::Impl::WriteStatistics(utils::statistics::Writer& writer) const {
    writer["was-last-parse-successful"] = stats_.was_last_parse_successful;
    writer["parse-errors"] = stats_.parse_errors;
}

DynamicConfig::NoblockSubscriber::NoblockSubscriber(DynamicConfig& config_component) noexcept
    : config_component_(config_component) {}

concurrent::AsyncEventSource<const dynamic_config::Snapshot&>& DynamicConfig::NoblockSubscriber::GetEventSource(
) noexcept {
    return config_component_.impl_->GetChannel();
}

DynamicConfig::DynamicConfig(const ComponentConfig& config, const ComponentContext& context)
    : DynamicConfigUpdatesSinkBase(config, context), impl_(std::make_unique<Impl>(config, context)) {
    if (!impl_->AreUpdatesEnabled()) {
        dynamic_config::impl::RegisterUpdater(*this, kName, kName);
        SetConfig(kName, impl_->GetDefaultDocsMap());
    }
}

DynamicConfig::~DynamicConfig() = default;

dynamic_config::Source DynamicConfig::GetSource() { return impl_->GetSource(); }

const dynamic_config::DocsMap& DynamicConfig::GetDefaultDocsMap() const { return impl_->GetDefaultDocsMap(); }

ComponentHealth DynamicConfig::GetComponentHealth() const { return impl_->GetComponentHealth(); }

void DynamicConfig::OnLoadingCancelled() { impl_->OnLoadingCancelled(); }

void DynamicConfig::SetConfig(std::string_view updater, dynamic_config::DocsMap&& value) {
    impl_->SetConfig(updater, std::move(value));
}

void DynamicConfig::SetConfig(std::string_view updater, const dynamic_config::DocsMap& value) {
    impl_->SetConfig(updater, dynamic_config::DocsMap{value});
}

void DynamicConfig::NotifyLoadingFailed(std::string_view updater, std::string_view error) {
    impl_->NotifyLoadingFailed(updater, error);
}

yaml_config::Schema DynamicConfig::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: Component that stores the runtime config.
additionalProperties: false
properties:
    updates-enabled:
        type: boolean
        description: should be set to 'true' if there is an updater component
        defaultDescription: false
    defaults:
        type: object
        description: optional values for configs that override the defaults specified in dynamic_config::Key definitions
        defaultDescription: values from dynamic_config::Key definitions are used
        properties: {}
        additionalProperties: true
    defaults-path:
        type: string
        description: optional file with config values that override the defaults specified in dynamic_config::Key definitions
        defaultDescription: values from dynamic_config::Key definitions are used
    fs-cache-path:
        type: string
        description: path to the file to read and dump a config cache; set to empty string to disable reading and dumping configs to FS
        defaultDescription: no fs cache
    fs-task-processor:
        type: string
        description: name of the task processor to run the blocking file write operations
        defaultDescription: required if defaults-path or fs-cache-path is specified
)");
}

}  // namespace components

USERVER_NAMESPACE_END
