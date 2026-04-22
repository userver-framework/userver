#include <userver/utils/statistics/system_statistics_collector.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/utils/statistics/system_statistics_collector.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

SystemStatisticsCollector::SystemStatisticsCollector(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      with_nginx_(config["with-nginx"].As<bool>(false)),
      fs_task_processor_(GetFsTaskProcessor(config, context)),
      periodic_(
          "system_statistics_collector",
          {std::chrono::seconds(10), {utils::PeriodicTask::Flags::kNow}},
          [this] { ProcessTimer(); }
      )
{
    utils::statistics::RegisterWriterScope(context, "", [this](utils::statistics::Writer& writer) {
        ExtendStatistics(writer);
    });
}

void SystemStatisticsCollector::ProcessTimer() {
    engine::CriticalAsyncNoSpan(fs_task_processor_, [&] {
        auto self = utils::statistics::impl::GetSelfSystemStatistics();
        utils::statistics::impl::SystemStats nginx;
        if (with_nginx_) {
            nginx = utils::statistics::impl::GetSystemStatisticsByExeName("nginx");
        }

        auto data = data_.UniqueLock();
        data->last_stats = self;
        data->last_nginx_stats = nginx;
    }).Get();
}

void SystemStatisticsCollector::ExtendStatistics(utils::statistics::Writer& writer) {
    auto data = data_.Lock();

    DumpMetric(writer, data->last_stats);
    if (with_nginx_) {
        writer.ValueWithLabels(data->last_nginx_stats, {"application", "nginx"});
    }
}

yaml_config::Schema SystemStatisticsCollector::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/utils/statistics/system_statistics_collector.yaml"
    );
}

}  // namespace components

USERVER_NAMESPACE_END
