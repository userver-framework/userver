#pragma once

/// @file utils/statistics/system_statistics_collector.hpp
/// @brief @copybrief components::SystemStatisticsCollector

#include <components/component_fwd.hpp>
#include <components/loggable_component_base.hpp>
#include <concurrent/variable.hpp>
#include <utils/periodic_task.hpp>
#include <utils/statistics/storage.hpp>
#include <utils/statistics/system_statistics.hpp>

namespace formats::json {
class Value;
}  // namespace formats::json

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for system resource usage statistics collection.
///
/// Periodically queries resource usage info and reports is as a set of metrics.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fs-task-processor | Task processor to use for statistics gathering | -
/// update-interval | Statistics collection interval | 1m
/// with-nginx | Whether to collect and report nginx processes statistics | false
///
/// Note that `with-nginx` is a relatively expensive option as it requires full
/// process list scan.
///
/// ## Configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample system statistics component config

// clang-format on

class SystemStatisticsCollector final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "system-statistics-collector";

  SystemStatisticsCollector(const ComponentConfig&, const ComponentContext&);

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest&);
  void UpdateStats();

  const bool with_nginx_;
  concurrent::Variable<utils::statistics::impl::SystemStats> self_stats_;
  concurrent::Variable<utils::statistics::impl::SystemStats> nginx_stats_;
  utils::statistics::Entry statistics_holder_;
  utils::PeriodicTask update_task_;
};

}  // namespace components
