#include <userver/engine/task_processors_load_monitor.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <components/manager.hpp>
#include <components/manager_config.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

TaskProcessor* GetMonitorTaskProcessor(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  static constexpr std::string_view kOurTaskProcessorFieldName{
      "task-processor"};
  static constexpr std::string_view kMonitoringTaskProcessorFieldName{
      "task_processor"};

  if (config.HasMember(kOurTaskProcessorFieldName)) {
    return &context.GetTaskProcessor(
        config[kOurTaskProcessorFieldName].As<std::string>());
  }

  static constexpr std::string_view kMonitorComponentName =
      server::handlers::ServerMonitor::kName;

  const auto& component_configs = context.GetManager().GetConfig().components;
  for (const auto& component_config : component_configs) {
    if (component_config.Name() == kMonitorComponentName) {
      return &context.GetTaskProcessor(
          component_config[kMonitoringTaskProcessorFieldName]
              .As<std::string>());
    }
  }

  return nullptr;
}

}  // namespace

class TaskProcessorsLoadMonitor::Impl final {
 public:
  Impl(const components::ComponentConfig& config,
       const components::ComponentContext& context) {
    auto* monitor_task_processor = GetMonitorTaskProcessor(config, context);
    if (!monitor_task_processor) {
      LOG_WARNING() << "TaskProcessorLoadMonitor is enabled, but neither "
                       "'task-processor' is specified in config, nor "
                       "server::handlers::ServerMonitor is enabled. The "
                       "monitoring will be no-op.";
      return;
    }

    const auto& task_processors_map =
        context.GetManager().GetTaskProcessorsMap();

    task_processors_.reserve(task_processors_map.size());
    for (const auto& [name, tp] : task_processors_map) {
      task_processors_.push_back(
          {*tp,
           // Number of threads in TaskProcessor is fixed, so we pre-initialize
           // the size needed to avoid synchronization.
           utils::FixedArray<TaskProcessorMeta::LoadValue>{tp->GetWorkerCount(),
                                                           0}});
    }

    {
      utils::PeriodicTask::Settings settings{kCollectInterval};
      settings.task_processor = monitor_task_processor;
      UASSERT(settings.task_processor);

      collector_.Start("task-processors-load-collector", settings,
                       [this] { CollectCurrentLoad(); });
    }

    auto& storage =
        context.FindComponent<components::StatisticsStorage>().GetStorage();
    statistics_holder_ = storage.RegisterWriter(
        "engine.task-processors-load-percent",
        [this](utils::statistics::Writer& writer) { ExtendWriter(writer); });
  }

  ~Impl() {
    statistics_holder_.Unregister();
    collector_.Stop();
  }

 private:
  static constexpr std::chrono::seconds kCollectInterval{5};

  struct TaskProcessorMeta final {
    using LoadValue = utils::statistics::RelaxedCounter<std::uint8_t>;

    const TaskProcessor& task_processor;
    utils::FixedArray<LoadValue> current_load;
  };

  void CollectCurrentLoad() noexcept {
    for (auto& tp_meta : task_processors_) {
      const auto current_load = tp_meta.task_processor.CollectCurrentLoadPct();
      // This should always be true, but just in case
      UASSERT(current_load.size() == tp_meta.current_load.size());
      if (current_load.size() == tp_meta.current_load.size()) {
        for (std::size_t i = 0; i < current_load.size(); ++i) {
          tp_meta.current_load[i].Store(current_load[i]);
        }
      }
    }
  }

  void ExtendWriter(utils::statistics::Writer& writer) const {
    for (const auto& tp_meta : task_processors_) {
      const utils::statistics::LabelView task_processor_label{
          "task_processor", tp_meta.task_processor.Name()};

      for (const auto& [index, value] :
           utils::enumerate(tp_meta.current_load)) {
        writer.ValueWithLabels(
            value.Load(),
            {task_processor_label, {"thread", std::to_string(index)}});
      }
    }
  }

  std::vector<TaskProcessorMeta> task_processors_;

  utils::statistics::Entry statistics_holder_;
  utils::PeriodicTask collector_;
};

TaskProcessorsLoadMonitor::TaskProcessorsLoadMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase{config, context},
      impl_{std::make_unique<Impl>(config, context)} {}

TaskProcessorsLoadMonitor::~TaskProcessorsLoadMonitor() = default;

yaml_config::Schema TaskProcessorsLoadMonitor::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: task-processors-load-monitor config
additionalProperties: false
properties:
    task-processor:
        type: string
        description: name of the TaskProcessor to run monitoring on
        defaultDescription: |
          task_processor of ServerMonitor or none, if ServerMonitor is absent
)");
}

}  // namespace engine

USERVER_NAMESPACE_END
