#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>

namespace components {

class Tracer final : public impl::ComponentBase {
 public:
  static constexpr auto kName = "tracer";

  Tracer(const ComponentConfig& config, const ComponentContext& context);
};

}  // namespace components
