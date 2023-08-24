#include <userver/storages/mongo/dist_lock_component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context,
    storages::mongo::Collection collection)
    : components::LoggableComponentBase(component_config, component_context) {
  auto lock_name = component_config["lockname"].As<std::string>();

  auto ttl = component_config["lock-ttl"].As<std::chrono::milliseconds>();
  auto mongo_timeout =
      component_config["mongo-timeout"].As<std::chrono::milliseconds>();
  auto optional_restart_delay =
      component_config["restart-delay"]
          .As<std::optional<std::chrono::milliseconds>>();
  const auto prolong_ratio = 10;

  if (mongo_timeout >= ttl / 2)
    throw std::runtime_error("mongo-timeout must be less than lock-ttl / 2");

  dist_lock::DistLockSettings settings{ttl / prolong_ratio, ttl / prolong_ratio,
                                       ttl, mongo_timeout};
  if (optional_restart_delay) {
    settings.worker_func_restart_delay = *optional_restart_delay;
  }

  auto strategy =
      std::make_shared<DistLockStrategy>(std::move(collection), lock_name);

  auto task_processor_name =
      component_config["task-processor"].As<std::optional<std::string>>();
  auto* task_processor =
      task_processor_name
          ? &component_context.GetTaskProcessor(task_processor_name.value())
          : nullptr;
  worker_ = std::make_unique<dist_lock::DistLockedWorker>(
      std::move(lock_name),
      [this]() {
        if (testsuite_enabled_) {
          DoWorkTestsuite();
        } else {
          DoWork();
        }
      },
      std::move(strategy), settings, task_processor);

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "distlock",
      [this](utils::statistics::Writer& writer) { writer = *worker_; },
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

void DistLockComponentBase::Start() {
  if (testsuite_enabled_) return;
  worker_->Start();
}

void DistLockComponentBase::Stop() { worker_->Stop(); }

yaml_config::Schema DistLockComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Base class for mongo-based distlock worker components
additionalProperties: false
properties:
    lockname:
        type: string
        description: name of the lock
    lock-ttl:
        type: string
        description: TTL of the lock; must be at least as long as the duration between subsequent cancellation checks, otherwise brain split is possible
    mongo-timeout:
        type: string
        description: timeout, must be at least 2*lock-ttl
    restart-delay:
        type: string
        description: how much time to wait after failed task restart
        defaultDescription: 100ms
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

}  // namespace storages::mongo

USERVER_NAMESPACE_END
