#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>

#include <userver/engine/subprocess/process_starter.hpp>

namespace components {

class ProcessStarter : public LoggableComponentBase {
 public:
  ProcessStarter(const ComponentConfig& config,
                 const ComponentContext& context);

  static constexpr const char* kName = "process-starter";

  engine::subprocess::ProcessStarter& Get() { return process_starter_; }

 private:
  engine::subprocess::ProcessStarter process_starter_;
};

}  // namespace components
