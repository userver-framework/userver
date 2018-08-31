#pragma once

#include <json/value.h>

#include "component_base.hpp"
#include "manager.hpp"

namespace components {

class MonitorableComponentBase : public ComponentBase {
 public:
  MonitorableComponentBase(const components::ComponentConfig& config,
                           const components::ComponentContext&)
      : default_metrics_path_(config.Name()) {}

  virtual ~MonitorableComponentBase() {}

  virtual Json::Value GetMonitorData(MonitorVerbosity verbosity) const = 0;

  virtual std::string GetMetricsPath() const { return default_metrics_path_; }

 private:
  std::string default_metrics_path_;
};

}  // namespace components
