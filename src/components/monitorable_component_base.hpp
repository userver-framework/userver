#pragma once

#include <json/value.h>

#include "component_base.hpp"
#include "manager.hpp"

namespace components {

class MonitorableComponentBase : public ComponentBase {
 public:
  virtual ~MonitorableComponentBase() {}

  virtual Json::Value GetMonitorData(MonitorVerbosity verbosity) const = 0;
};

}  // namespace components
