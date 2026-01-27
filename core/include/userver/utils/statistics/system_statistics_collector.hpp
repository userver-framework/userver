#pragma once

/// @file userver/utils/statistics/system_statistics_collector.hpp
/// @brief @copybrief components::SystemStatisticsCollector

#include <userver/components/component_base.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/periodic_task.hpp>
#include <utils/statistics/system_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component for system resource usage statistics collection.
///
/// Periodically queries resource usage info and reports is as a set of metrics.
///
/// ## Static options of components::SystemStatisticsCollector :
/// @include{doc} scripts/docs/en/components_schema/core/src/utils/statistics/system_statistics_collector.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample system statistics component config
class SystemStatisticsCollector final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::SystemStatisticsCollector
    static constexpr std::string_view kName = "system-statistics-collector";

    SystemStatisticsCollector(const ComponentConfig&, const ComponentContext&);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void ExtendStatistics(utils::statistics::Writer& writer);

    void ProcessTimer();

    struct Data {
        utils::statistics::impl::SystemStats last_stats{};
        utils::statistics::impl::SystemStats last_nginx_stats{};
    };

    const bool with_nginx_;
    engine::TaskProcessor& fs_task_processor_;
    concurrent::Variable<Data> data_;
    utils::PeriodicTask periodic_;
};

template <>
inline constexpr bool kHasValidate<SystemStatisticsCollector> = true;

}  // namespace components

USERVER_NAMESPACE_END
