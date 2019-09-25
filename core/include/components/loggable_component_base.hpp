#pragma once

#include <components/impl/component_base.hpp>

namespace components {

class ComponentContext;
class ComponentConfig;

/// Base class for all application components, it depends on logger and tracer
class LoggableComponentBase : public impl::ComponentBase {
 public:
  LoggableComponentBase(const ComponentConfig&, const ComponentContext&);
};

}  // namespace components
