#include <userver/utils/statistics/system_statistics_collector.hpp>

#include <memory>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/async.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <utils/statistics/system_statistics.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/utils/statistics/system_statistics_collector.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

struct SystemStatisticsCollector::Impl {
    struct Data {
        utils::statistics::impl::SystemStats last_stats{};
        utils::statistics::impl::SystemStats last_nginx_stats{};
    };

    Impl(const ComponentConfig& config, const ComponentContext& context)
        : with_nginx(config["with-nginx"].As<bool>(false)),
          fs_task_processor(GetFsTaskProcessor(config, context))
    {
        periodic.Start(
            "system_statistics_collector",
            {std::chrono::seconds(10), {utils::PeriodicTask::Flags::kNow}},
            [this] { ProcessTimer(); }
        );
    }

    void ProcessTimer();

    const bool with_nginx;
    engine::TaskProcessor& fs_task_processor;
    concurrent::Variable<Data> data;
    utils::PeriodicTask periodic;
};

void SystemStatisticsCollector::Impl::ProcessTimer() {
    engine::CriticalAsyncNoTracing(fs_task_processor, [&] {
        auto self = utils::statistics::impl::GetSelfSystemStatistics();
        utils::statistics::impl::SystemStats nginx;
        if (with_nginx) {
            nginx = utils::statistics::impl::GetSystemStatisticsByExeName("nginx");
        }

        auto data_lock = data.UniqueLock();
        data_lock->last_stats = self;
        data_lock->last_nginx_stats = nginx;
    }).Get();
}

SystemStatisticsCollector::SystemStatisticsCollector(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      impl_(std::make_unique<Impl>(config, context))
{
    utils::statistics::RegisterWriterScope(context, "", [this](utils::statistics::Writer& writer) {
        ExtendStatistics(writer);
    });
}

SystemStatisticsCollector::~SystemStatisticsCollector() = default;

void SystemStatisticsCollector::ExtendStatistics(utils::statistics::Writer& writer) {
    auto data = impl_->data.Lock();

    DumpMetric(writer, data->last_stats);
    if (impl_->with_nginx) {
        writer.ValueWithLabels(data->last_nginx_stats, {"application", "nginx"});
    }
}

yaml_config::Schema SystemStatisticsCollector::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/utils/statistics/system_statistics_collector.yaml"
    );
}

}  // namespace components

USERVER_NAMESPACE_END
