#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
class ProcessStarter : public LoggableComponentBase {
 public:
  ProcessStarter(const ComponentConfig& config,
                 const ComponentContext& context);

  static constexpr std::string_view kName = "process-starter";

  engine::subprocess::ProcessStarter& Get() { return process_starter_; }

 private:
  engine::subprocess::ProcessStarter process_starter_;
};

}  // namespace components

USERVER_NAMESPACE_END
