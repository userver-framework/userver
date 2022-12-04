#include <userver/utils/statistics/system_statistics_collector.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <utils/statistics/system_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

SystemStatisticsCollector::SystemStatisticsCollector(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      with_nginx_(config["with-nginx"].As<bool>(false)),
      fs_task_processor_(context.GetTaskProcessor(
          config["fs-task-processor"].As<std::string>())) {
  statistics_holder_ =
      context.FindComponent<components::StatisticsStorage>()
          .GetStorage()
          .RegisterWriter("", [this](utils::statistics::Writer& writer) {
            ExtendStatistics(writer);
          });
}

SystemStatisticsCollector::~SystemStatisticsCollector() {
  statistics_holder_.Unregister();
}

void SystemStatisticsCollector::ExtendStatistics(
    utils::statistics::Writer& writer) {
  engine::CriticalAsyncNoSpan(fs_task_processor_, [&] {
    DumpMetric(writer, utils::statistics::impl::GetSelfSystemStatistics());
    if (with_nginx_) {
      writer.ValueWithLabels(
          utils::statistics::impl::GetSystemStatisticsByExeName("nginx"),
          {"application", "nginx"});
    }
  }).Get();
}

yaml_config::Schema SystemStatisticsCollector::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component for system resource usage statistics collection.
additionalProperties: false
properties:
    fs-task-processor:
        type: string
        description: Task processor to use for statistics gathering
    with-nginx:
        type: boolean
        description: Whether to collect and report nginx processes statistics
        defaultDescription: false
)");
}

}  // namespace components

USERVER_NAMESPACE_END
