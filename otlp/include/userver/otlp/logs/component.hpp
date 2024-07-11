#pragma once

/// @file userver/otlp/logs/component.hpp
/// @brief @copybrief otlp::logs::Component

#include <memory>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>

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
};

}  // namespace otlp

USERVER_NAMESPACE_END
