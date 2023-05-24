#include <userver/storages/postgres/dist_lock_component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context)
    : components::LoggableComponentBase(component_config, component_context) {
  auto cluster = component_context
                     .FindComponent<components::Postgres>(
                         component_config["cluster"].As<std::string>())
                     .GetCluster();
  auto table = component_config["table"].As<std::string>();
  auto lock_name = component_config["lockname"].As<std::string>();

  auto ttl = component_config["lock-ttl"].As<std::chrono::milliseconds>();
  auto pg_timeout =
      component_config["pg-timeout"].As<std::chrono::milliseconds>();
  const auto prolong_ratio = 10;

  if (pg_timeout >= ttl / 2)
    throw std::runtime_error("pg-timeout must be less than lock-ttl / 2");

  dist_lock::DistLockSettings settings{ttl / prolong_ratio, ttl / prolong_ratio,
                                       ttl, pg_timeout};
  settings.worker_func_restart_delay =
      component_config["restart-delay"].As<std::chrono::milliseconds>(
          settings.worker_func_restart_delay);

  auto strategy = std::make_shared<DistLockStrategy>(std::move(cluster), table,
                                                     lock_name, settings);

  auto task_processor_name =
      component_config["task-processor"].As<std::optional<std::string>>();
  auto* task_processor =
      task_processor_name
          ? &component_context.GetTaskProcessor(task_processor_name.value())
          : nullptr;
  worker_ = std::make_unique<dist_lock::DistLockedWorker>(
      lock_name,
      [this]() {
        if (testsuite_enabled_) {
          DoWorkTestsuite();
        } else {
          DoWork();
        }
      },
      std::move(strategy), settings, task_processor);

  autostart_ = component_config["autostart"].As<bool>(false);

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "distlock",
      [this](USERVER_NAMESPACE::utils::statistics::Writer& writer) {
        writer = *worker_;
      },
      {{"distlock_name", component_config.Name()}});

  if (component_config["testsuite-support"].As<bool>(false)) {
    auto& testsuite_tasks = testsuite::GetTestsuiteTasks(component_context);

    if (testsuite_tasks.IsEnabled()) {
      testsuite_tasks.RegisterTask("distlock/" + component_config.Name(),
                                   [this] { worker_->RunOnce(); });
      testsuite_enabled_ = true;
    }
  }
}

DistLockComponentBase::~DistLockComponentBase() {
  statistics_holder_.Unregister();
}

dist_lock::DistLockedWorker& DistLockComponentBase::GetWorker() {
  return *worker_;
}

void DistLockComponentBase::AutostartDistLock() {
  if (testsuite_enabled_) return;
  if (autostart_) worker_->Start();
}

void DistLockComponentBase::StopDistLock() { worker_->Stop(); }

yaml_config::Schema DistLockComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Base class for postgres-based distlock worker components
additionalProperties: false
properties:
    cluster:
        type: string
        description: postgres cluster name
    table:
        type: string
        description: table name to store distlocks
    lockname:
        type: string
        description: name of the lock
    lock-ttl:
        type: string
        description: TTL of the lock; must be at least as long as the duration between subsequent cancellation checks, otherwise brain split is possible
    pg-timeout:
        type: string
        description: timeout, must be less than lock-ttl/2
    restart-delay:
        type: string
        description: how much time to wait after failed task restart
        defaultDescription: 100ms
    autostart:
        type: boolean
        description: if true, start automatically after component load
        defaultDescription: true
    task-processor:
        type: string
        description: the name of the TaskProcessor for running DoWork
        defaultDescription: main-task-processor
    testsuite-support:
        type: boolean
        description: Enable testsuite support
        defaultDescription: false
)");
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
