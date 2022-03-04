#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

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

  static std::string GetStaticConfigSchema();

 private:
  engine::subprocess::ProcessStarter process_starter_;
};

template <>
inline constexpr bool kHasValidate<ProcessStarter> = true;

}  // namespace components

USERVER_NAMESPACE_END
