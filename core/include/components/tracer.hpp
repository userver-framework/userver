#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

namespace components {

class Tracer final : public ComponentBase {
 public:
  static constexpr auto kName = "tracer";

  Tracer(const ComponentConfig& config, const ComponentContext& context);
};

}  // namespace components
