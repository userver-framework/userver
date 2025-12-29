#pragma once

/// @file userver/otlp/logs/component.hpp
/// @brief @copybrief otlp::LoggerComponent

#include <memory>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

class Logger;

/// @ingroup userver_components
///
/// @brief Component to configure logging via OTLP collector.
///
/// ## Static options of otlp::LoggerComponent :
/// @include{doc} scripts/docs/en/components_schema/otlp/src/otlp/logs/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// Possible sink values:
/// * `otlp`: OTLP exporter
/// * `default`: _default_ logger from the `logging` component
/// * `both`: _default_ logger and OTLP exporter
class LoggerComponent final : public components::RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of otlp::LoggerComponent
    static constexpr std::string_view kName = "otlp-logger";

    LoggerComponent(const components::ComponentConfig&, const components::ComponentContext&);

    ~LoggerComponent();

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::shared_ptr<Logger> logger_;
    logging::LoggerRef old_logger_;
    utils::statistics::Entry statistics_holder_;
};

}  // namespace otlp

USERVER_NAMESPACE_END
