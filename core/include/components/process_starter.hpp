#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>

#include <engine/subprocess/process_starter.hpp>

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
