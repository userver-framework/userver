#pragma once

/// @file userver/components/logging_configurator.hpp
/// @brief @copybrief components::LoggingConfigurator

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <dynamic_config/variables/USERVER_LOG_DYNAMIC_DEBUG.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
struct DynamicDebugConfig;
}

namespace components {

/// @ingroup userver_components
///
/// @brief Helper component to configure logging.
///
/// The functionality is not in Trace or Logger components because that
/// introduces circular dependency between Logger and DynamicConfig.
///
/// ## LoggingConfigurator Dynamic config
/// * @ref USERVER_LOG_DYNAMIC_DEBUG
/// * @ref USERVER_NO_LOG_SPANS
///
/// ## Static options of components::LoggingConfigurator :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/logging_configurator.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Config example:
///
/// @snippet components/common_component_list_test.cpp Sample logging configurator component config
class LoggingConfigurator final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::LoggingConfigurator component
    static constexpr std::string_view kName = "logging-configurator";

    LoggingConfigurator(const ComponentConfig& config, const ComponentContext& context);

    ~LoggingConfigurator() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config);

    utils::statistics::MetricsStoragePtr metrics_storage_;
    // config_subscription_ must be the last field!
    concurrent::AsyncEventSubscriberScope config_subscription_;
};

/// }@

template <>
inline constexpr bool kHasValidate<LoggingConfigurator> = true;

}  // namespace components

USERVER_NAMESPACE_END
