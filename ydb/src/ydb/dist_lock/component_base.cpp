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

// YDB headers leak `ARCADIA_ROOT` macro, so we use __has_include()
#if __has_include("generated/src/ydb/dist_lock/component_base.yaml.hpp")
#include "generated/src/ydb/dist_lock/component_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

struct NodeSettings final {
    std::chrono::milliseconds session_grace_period{10'000};
};

NodeSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<NodeSettings>) {
    NodeSettings result;
    result
        .session_grace_period = config["session-grace-period"].As<std::chrono::milliseconds>(result.session_grace_period
    );
    return result;
}

void TryCreateNode(
    CoordinationClient& coordination_client,
    const std::string& coordination_node,
    const NodeSettings& node_settings
) {
    NYdb::NCoordination::TCreateNodeSettings create_node_settings;
    create_node_settings.SessionGracePeriod(node_settings.session_grace_period);

    try {
        coordination_client.CreateNode(coordination_node, create_node_settings);
    } catch (const YdbResponseError& ex) {
        LOG_WARNING() << "Could not create coordination node: " << ex;
    }
}

void TryCreateSemaphore(
    CoordinationSession& session,
    const std::string& semaphore_name,
    std::uint64_t semaphore_limit
) {
    try {
        session.CreateSemaphore(semaphore_name, semaphore_limit);
    } catch (const YdbResponseError& ex) {
        LOG_WARNING() << "Could not create semaphore: " << ex;
    }
}

void InitialSetup(
    CoordinationClient& coordination_client,
    const std::string& coordination_node,
    const std::string& semaphore_name,
    const NodeSettings& node_settings
) {
    TryCreateNode(coordination_client, coordination_node, node_settings);

    auto session = coordination_client.StartSession(coordination_node, {});
    TryCreateSemaphore(session, semaphore_name, impl::dist_lock::kSemaphoreLimit);
    session.Close();
}

}  // namespace

DistLockComponentBase::DistLockComponentBase(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context
)
    : components::ComponentBase(component_config, component_context),
      testsuite_tasks_(testsuite::GetTestsuiteTasks(component_context)),
      testsuite_task_name_("distlock/" + component_config.Name())
{
    const auto semaphore_name = component_config["semaphore-name"].As<std::string>();
    const auto database_settings = component_config["database-settings"];
    const auto database = database_settings["dbname"].As<std::string>();
    const auto coordination_node = database_settings["coordination-node"].As<std::string>();

    auto coordination_client = component_context.FindComponent<YdbComponent>().GetCoordinationClient(database);

    const auto node_settings = component_config["node-settings"].As<NodeSettings>();

    if (component_config["initial-setup"].As<bool>(true)) {
        InitialSetup(*coordination_client, coordination_node, semaphore_name, node_settings);
    }

    const auto task_processor_name = component_config["task-processor"].As<std::optional<std::string>>();
    auto& task_processor =
        task_processor_name.has_value()
            ? component_context.GetTaskProcessor(*task_processor_name)
            : engine::current_task::GetTaskProcessor();

    worker_.emplace(
        task_processor,
        std::move(coordination_client),
        coordination_node,
        semaphore_name,
        component_config.As<DistLockSettings>(),
        [this] {
            if (testsuite_tasks_.IsEnabled()) {
                DoWorkTestsuite();
            } else {
                DoWork();
            }
        }
    );

    utils::statistics::RegisterWriterScope(
        component_context,
        "distlock",
        [this](utils::statistics::Writer& writer) { writer = *worker_; },
        {{"distlock_name", component_config.Name()}}
    );
}

bool DistLockComponentBase::OwnsLock() const noexcept { return worker_->OwnsLock(); }

void DistLockComponentBase::Start() {
    if (testsuite_tasks_.IsEnabled()) {
        testsuite_tasks_.RegisterTask(testsuite_task_name_, [this] { worker_->RunOnce(); });
        return;
    }
    worker_->Start();
}

void DistLockComponentBase::Stop() noexcept { worker_->Stop(); }

yaml_config::Schema DistLockComponentBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/ydb/dist_lock/component_base.yaml");
}

}  // namespace ydb

USERVER_NAMESPACE_END
