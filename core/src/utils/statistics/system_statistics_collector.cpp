#include <userver/utils/statistics/system_statistics_collector.hpp>

#include <chrono>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
namespace {
constexpr auto kDefaultStatsUpdateInterval = std::chrono::minutes{1};
}  // namespace

SystemStatisticsCollector::SystemStatisticsCollector(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      with_nginx_(config["with-nginx"].As<bool>(false)),
      statistics_holder_(context.FindComponent<components::StatisticsStorage>()
                             .GetStorage()
                             .RegisterExtender({}, [this](const auto& request) {
                               return ExtendStatistics(request);
                             })) {
  const auto fs_task_processor_name =
      config["fs-task-processor"].As<std::string>();
  const auto stats_update_period =
      config["update-interval"].As<std::chrono::seconds>(
          kDefaultStatsUpdateInterval);
  engine::AsyncNoSpan(context.GetTaskProcessor(fs_task_processor_name),
                      [this, stats_update_period] {
                        update_task_.Start(
                            kName,
                            {stats_update_period,
                             {utils::PeriodicTask::Flags::kNow,
                              utils::PeriodicTask::Flags::kStrong}},
                            [this] { UpdateStats(); });
                      })
      .Get();
}

formats::json::Value SystemStatisticsCollector::ExtendStatistics(
    const utils::statistics::StatisticsRequest& request) {
  formats::json::ValueBuilder stats;
  if (request.prefix.empty()) {
    auto self_locked = self_stats_.Lock();
    stats = *self_locked;
  }

  if (with_nginx_ && (boost::algorithm::starts_with(request.prefix, "nginx") ||
                      boost::algorithm::starts_with("nginx", request.prefix))) {
    auto nginx_stats = stats["nginx"];
    {
      auto nginx_locked = nginx_stats_.Lock();
      nginx_stats = *nginx_locked;
    }
    utils::statistics::SolomonLabelValue(nginx_stats, "application");
  }
  return stats.ExtractValue();
}

void SystemStatisticsCollector::UpdateStats() {
  auto self_current = utils::statistics::impl::GetSelfSystemStatistics();
  {
    auto self_locked = self_stats_.Lock();
    *self_locked = self_current;
  }
  if (with_nginx_) {
    auto nginx_current =
        utils::statistics::impl::GetSystemStatisticsByExeName("nginx");
    {
      auto nginx_locked = nginx_stats_.Lock();
      *nginx_locked = nginx_current;
    }
  }
}

}  // namespace components

USERVER_NAMESPACE_END
