#include <userver/ydb/dist_lock/component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/ydb/component.hpp>
#include <userver/ydb/dist_lock/settings.hpp>
#include <userver/ydb/exceptions.hpp>

#include <ydb/impl/dist_lock/semaphore_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

struct NodeSettings final {
  std::chrono::milliseconds session_grace_period{10'000};
};

NodeSettings Parse(const yaml_config::YamlConfig& config,
                   formats::parse::To<NodeSettings>) {
  NodeSettings result;
  result.session_grace_period =
      config["session-grace-period"].As<std::chrono::milliseconds>(
          result.session_grace_period);
  return result;
}

void TryCreateNode(CoordinationClient& coordination_client,
                   const std::string& coordination_node,
                   const NodeSettings& node_settings) {
  NYdb::NCoordination::TCreateNodeSettings create_node_settings;
  create_node_settings.SessionGracePeriod(node_settings.session_grace_period);

  try {
    coordination_client.CreateNode(coordination_node, create_node_settings);
  } catch (const YdbResponseError& ex) {
    LOG_WARNING() << "Could not create coordination node: " << ex;
  }
}

void TryCreateSemaphore(CoordinationSession& session,
                        const std::string& semaphore_name,
                        std::uint64_t semaphore_limit) {
  try {
    session.CreateSemaphore(semaphore_name, semaphore_limit);
  } catch (const YdbResponseError& ex) {
    LOG_WARNING() << "Could not create semaphore: " << ex;
  }
}

void InitialSetup(CoordinationClient& coordination_client,
                  const std::string& coordination_node,
                  const std::string& semaphore_name,
                  const NodeSettings& node_settings) {
  TryCreateNode(coordination_client, coordination_node, node_settings);

  auto session = coordination_client.StartSession(coordination_node, {});
  TryCreateSemaphore(session, semaphore_name, impl::dist_lock::kSemaphoreLimit);
  session.Close();
}

}  // namespace

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context)
    : components::LoggableComponentBase(component_config, component_context),
      testsuite_tasks_(testsuite::GetTestsuiteTasks(component_context)),
      testsuite_task_name_("distlock/" + component_config.Name()) {
  const auto semaphore_name =
      component_config["semaphore-name"].As<std::string>();
  const auto database_settings = component_config["database-settings"];
  const auto database = database_settings["dbname"].As<std::string>();
  const auto coordination_node =
      database_settings["coordination-node"].As<std::string>();

  auto coordination_client =
      component_context.FindComponent<YdbComponent>().GetCoordinationClient(
          database);

  const auto node_settings =
      component_config["node-settings"].As<NodeSettings>();

  if (component_config["initial-setup"].As<bool>(true)) {
    InitialSetup(*coordination_client, coordination_node, semaphore_name,
                 node_settings);
  }

  const auto task_processor_name =
      component_config["task-processor"].As<std::optional<std::string>>();
  auto& task_processor =
      task_processor_name.has_value()
          ? component_context.GetTaskProcessor(*task_processor_name)
          : engine::current_task::GetTaskProcessor();

  worker_.emplace(task_processor, std::move(coordination_client),
                  coordination_node, semaphore_name,
                  component_config.As<DistLockSettings>(), [this] {
                    if (testsuite_tasks_.IsEnabled()) {
                      DoWorkTestsuite();
                    } else {
                      DoWork();
                    }
                  });

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "distlock",
      [this](utils::statistics::Writer& writer) { writer = *worker_; },
      {{"distlock_name", component_config.Name()}});
}

DistLockComponentBase::~DistLockComponentBase() {
  statistics_holder_.Unregister();
}

bool DistLockComponentBase::OwnsLock() const noexcept {
  return worker_->OwnsLock();
}

void DistLockComponentBase::Start() {
  if (testsuite_tasks_.IsEnabled()) {
    testsuite_tasks_.RegisterTask(testsuite_task_name_,
                                  [this] { worker_->RunOnce(); });
    return;
  }
  worker_->Start();
}

void DistLockComponentBase::Stop() noexcept { worker_->Stop(); }

yaml_config::Schema DistLockComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: YDB distlock component
additionalProperties: false
properties:
    semaphore-name:
        type: string
        description: name of the semaphore within the coordination node
    database-settings:
        type: object
        description: settings that might be used for a group of related distlock instances
        additionalProperties: false
        properties:
            dbname:
                type: string
                description: the key of the database within ydb component (NOT the actual database path)
            coordination-node:
                type: string
                description: name of the coordination node within the database
    initial-setup:
        type: boolean
        description: if true, then create the coordination node and the semaphore unless they already exist
        defaultDescription: true
    task-processor:
        type: string
        description: the name of the TaskProcessor for running DoWork
        defaultDescription: the default TaskProcessor, typically main-task-processor
    node-settings:
        type: object
        description: settings for coordination node creation
        additionalProperties: false
        properties:
            session-grace-period:
                type: string
                description: |
                    the time after which the lock will be given to another host
                    after a network failure; this timer starts
                    on the coordination node conceptually at the same time
                    as 'session-timeout' timer starts on the service instance
    session-timeout:
        type: string
        description: for how long we will try to restore session after a network failure before dropping it
        defaultDescription: 5s
    restart-session-delay:
        type: string
        description: backoff before attempting to reconnect session after it returns "permanent failure"
        defaultDescription: 1s
    acquire-interval:
        type: string
        description: backoff before repeating a failed Acquire call
        defaultDescription: 100ms
    restart-delay:
        type: string
        description: backoff before calling DoWork again after it returns or throws
        defaultDescription: 100ms
    cancel-task-time-limit:
        type: string
        description: time, within which a cancelled DoWork is expected to finish
        defaultDescription: 5s
  )");
}

}  // namespace ydb

USERVER_NAMESPACE_END
