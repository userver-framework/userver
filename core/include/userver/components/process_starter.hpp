#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// The component does **not** have any options for service config.
class ProcessStarter : public LoggableComponentBase {
 public:
  ProcessStarter(const ComponentConfig& config,
                 const ComponentContext& context);

  static constexpr std::string_view kName = "process-starter";

  engine::subprocess::ProcessStarter& Get() { return process_starter_; }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  engine::subprocess::ProcessStarter process_starter_;
};

template <>
inline constexpr bool kHasValidate<ProcessStarter> =
    false;  // TODO: replace to true TAXICOMMON-4601

}  // namespace components

USERVER_NAMESPACE_END
