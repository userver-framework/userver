#pragma once

/// @file userver/components/logging_configurator.hpp
/// @brief @copybrief components::LoggingConfigurator

#include <memory>
#include <string_view>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <dynamic_config/variables/USERVER_LOG_DYNAMIC_DEBUG.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
struct DynamicDebugConfig;
struct TelemetryConfig;
}  // namespace logging

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
/// * @ref USERVER_TELEMETRY_SETTINGS
///
/// ## Static options of components::LoggingConfigurator :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/logging_configurator.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Config example:
///
/// @snippet core/src/components/common_component_list_test.cpp Sample logging configurator component config
class LoggingConfigurator final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::LoggingConfigurator component
    static constexpr std::string_view kName = "logging-configurator";

    LoggingConfigurator(const ComponentConfig& config, const ComponentContext& context);

    ~LoggingConfigurator() override;

    /// Replaces the process's single external policy and applies the stricter settings
    /// from it and @ref USERVER_TELEMETRY_SETTINGS. An empty policy removes external restrictions.
    /// Calls are synchronized with dynamic config updates. The caller must stop its
    /// subscriptions and clear its policy before this component is destroyed.
    /// @throws std::runtime_error for invalid sampling rates, without changing the policy.
    void SetRestrictions(logging::TelemetryConfig restrictions);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    class TelemetryState;

    void OnConfigUpdate(const dynamic_config::Diff& diff);

    utils::statistics::MetricsStoragePtr metrics_storage_;
    const std::unique_ptr<TelemetryState> telemetry_;
};

/// }@

template <>
inline constexpr bool kHasValidate<LoggingConfigurator> = true;

}  // namespace components

USERVER_NAMESPACE_END
