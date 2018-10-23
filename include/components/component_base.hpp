#pragma once

namespace components {

class ComponentContext;
class ComponentConfig;

/* Don't use it for application components, use LoggableComponentBase instead */
class ComponentBase {
 public:
  ComponentBase() = default;

  ComponentBase(ComponentBase&&) = delete;

  ComponentBase(const ComponentBase&) = delete;

  virtual ~ComponentBase() {}

  virtual void OnAllComponentsLoaded() {}

  virtual void OnAllComponentsAreStopping() {}
};

/* Base class for all application components, it depends on logger and tracer */
class LoggableComponentBase : public ComponentBase {
 public:
  LoggableComponentBase(const ComponentConfig&, const ComponentContext&);
};

}  // namespace components
