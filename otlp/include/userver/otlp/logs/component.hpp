#pragma once

/// @file userver/otlp/logs/component.hpp
/// @brief @copybrief otlp::logs::Component

#include <memory>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

class Logger;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component to configure logging via OTLP collector.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// endpoint | URI of otel collector (e.g. 127.0.0.1:4317) | -
/// max-queue-size | Maximum async queue size | 65535
/// max-batch-delay | Maximum batch delay | 100ms
/// service-name | Service name | unknown_service
/// attributes | Extra attributes for OTLP, object of key/value strings | -
/// sinks | List of sinks | -
/// sinks.logs | sink for logs (default|otlp|both) | otlp
/// sinks.tracing | sink for tracing (default|otlp|both) | otlp
///
/// Possible sink values:
/// * `otlp`: OTLP exporter
/// * `default`: _default_ logger from the `logging` component
/// * `both`: _default_ logger and OTLP exporter

// clang-format on
class LoggerComponent final : public components::RawComponentBase {
 public:
  static constexpr std::string_view kName = "otlp-logger";

  LoggerComponent(const components::ComponentConfig&,
                  const components::ComponentContext&);

  ~LoggerComponent();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::shared_ptr<Logger> logger_;
  logging::LoggerRef old_logger_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace otlp

USERVER_NAMESPACE_END
