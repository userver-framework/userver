#pragma once

namespace components {

class ComponentBase {
 public:
  virtual ~ComponentBase() {}

  virtual void OnAllComponentsLoaded() {}

  virtual void OnAllComponentsAreStopping() {}
};

}  // namespace components
