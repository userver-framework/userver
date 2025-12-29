#include <userver/storages/postgres/dist_lock_component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/scope.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/postgres/dist_lock_component_base.yaml.hpp"  // Y_IGNORE
#endif

#include <dynamic_config/variables/POSTGRES_DISTLOCK_SETTINGS.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context
)
    : components::ComponentBase(component_config, component_context),
      config_(component_context.FindComponent<components::DynamicConfig>().GetSource()),
      name_(component_config.Name()),
      real_host_name_(hostinfo::blocking::GetRealHostName())
{
    auto shard_number = component_config["shard-number"].As<size_t>(components::Postgres::kDefaultShardNumber);
    auto cluster =
        component_context.FindComponent<components::Postgres>(component_config["cluster"].As<std::string>())
            .GetClusterForShard(shard_number);
    auto table = component_config["table"].As<std::string>();
    auto lock_name = component_config["lockname"].As<std::string>();

    auto ttl = component_config["lock-ttl"].As<std::chrono::milliseconds>();
    auto pg_timeout = component_config["pg-timeout"].As<std::chrono::milliseconds>();
    const auto prolong_ratio = 10;

    if (pg_timeout >= ttl / 2) {
        throw std::runtime_error("pg-timeout must be less than lock-ttl / 2");
    }

    dist_lock::DistLockSettings settings{ttl / prolong_ratio, ttl / prolong_ratio, ttl, pg_timeout};
    settings.worker_func_restart_delay =
        component_config["restart-delay"].As<std::chrono::milliseconds>(settings.worker_func_restart_delay);

    default_settings_ = settings;
    auto strategy = std::make_shared<DistLockStrategy>(std::move(cluster), table, lock_name, settings);

    auto task_processor_name = component_config["task-processor"].As<std::optional<std::string>>();
    auto* task_processor =
        task_processor_name ? &component_context.GetTaskProcessor(task_processor_name.value()) : nullptr;

    auto locker_log_level = logging::LevelFromString(component_config["locker-log-level"].As<std::string>("info"));
    worker_ = std::make_unique<dist_lock::DistLockedWorker>(
        lock_name,
        [this]() {
            const auto snapshot = config_.GetSnapshot();
            if (ShouldRunOnHost(snapshot)) {
                if (testsuite_enabled_) {
                    DoWorkTestsuite();
                } else {
                    DoWork();
                }
            }
        },
        std::move(strategy),
        settings,
        task_processor,
        locker_log_level
    );
    subscription_token_ = config_.UpdateAndListen(this, name_, &DistLockComponentBase::OnConfigUpdate);
    autostart_ = component_config["autostart"].As<bool>(false);

    utils::statistics::RegisterWriterScope(
        component_context,
        "distlock",
        [this](USERVER_NAMESPACE::utils::statistics::Writer& writer) { writer = *worker_; },
        {{"distlock_name", component_config.Name()}}
    );

    if (component_config["testsuite-support"].As<bool>(false)) {
        auto& testsuite_tasks = testsuite::GetTestsuiteTasks(component_context);

        if (testsuite_tasks.IsEnabled()) {
            testsuite_tasks.RegisterTask("distlock/" + component_config.Name(), [this] { worker_->RunOnce(); });
            testsuite_enabled_ = true;
        }
    }
}

DistLockComponentBase::~DistLockComponentBase() { subscription_token_.Unsubscribe(); }

dist_lock::DistLockedWorker& DistLockComponentBase::GetWorker() { return *worker_; }

bool DistLockComponentBase::OwnsLock() const noexcept { return worker_->OwnsLock() || testsuite_enabled_; }

void DistLockComponentBase::AutostartDistLock() {
    if (testsuite_enabled_) {
        return;
    }
    if (autostart_) {
        worker_->Start();
    }
}

void DistLockComponentBase::StopDistLock() { worker_->Stop(); }

void DistLockComponentBase::OnConfigUpdate(const dynamic_config::Diff& diff) {
    const auto& old_snapshot_opt = diff.previous;
    const auto& new_snapshot = diff.current;

    const auto& new_settings = new_snapshot[::dynamic_config::POSTGRES_DISTLOCK_SETTINGS];
    const auto* new_overrides = utils::FindOrNullptr(new_settings.extra, name_);

    if (old_snapshot_opt.has_value()) {
        const auto& old_settings = (*old_snapshot_opt)[::dynamic_config::POSTGRES_DISTLOCK_SETTINGS];
        const auto* old_overrides = utils::FindOrNullptr(old_settings.extra, name_);

        if (!old_overrides && !new_overrides) {
            return;
        }
        if (old_overrides && new_overrides && *old_overrides == *new_overrides) {
            return;
        }
    }
    auto& worker = GetWorker();
    if (new_overrides) {
        dist_lock::DistLockSettings settings;
        settings.acquire_interval = new_overrides->acquire_interval.value_or(default_settings_.acquire_interval);
        settings.prolong_interval = new_overrides->prolong_interval.value_or(default_settings_.prolong_interval);
        settings.lock_ttl = new_overrides->lock_ttl.value_or(default_settings_.lock_ttl);
        settings.forced_stop_margin = new_overrides->forced_stop_margin.value_or(default_settings_.forced_stop_margin);
        settings.worker_func_restart_delay =
            new_overrides->worker_func_restart_delay.value_or(default_settings_.worker_func_restart_delay);
        settings.is_enabled = ShouldRunOnHost(new_snapshot);

        worker.UpdateSettings(settings);
    } else {
        worker.UpdateSettings(default_settings_);
    }
}
bool DistLockComponentBase::ShouldRunOnHost(const dynamic_config::Snapshot& config) const {
    const auto& worker_settings_config = config[::dynamic_config::POSTGRES_DISTLOCK_SETTINGS];
    if (const auto* settings = utils::FindOrNullptr(worker_settings_config.extra, name_)) {
        if (const auto& hosts = settings->run_on_hosts) {
            return hosts->count(real_host_name_);
        }
    }
    return true;
}

bool DistLockComponentBase::IsCancelAdvised() const { return worker_->IsCancelAdvised(); }

yaml_config::Schema DistLockComponentBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        components::ComponentBase>("src/storages/postgres/dist_lock_component_base.yaml");
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
