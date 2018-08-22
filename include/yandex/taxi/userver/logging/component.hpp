#pragma once

#include <string>
#include <unordered_map>

#include <yandex/taxi/userver/components/component_base.hpp>
#include <yandex/taxi/userver/components/component_config.hpp>
#include <yandex/taxi/userver/components/component_context.hpp>

#include "logger.hpp"

namespace components {

class Logging : public ComponentBase {
 public:
  static constexpr const char* kName = "logging";

  Logging(const ComponentConfig&, const ComponentContext&);

  logging::LoggerPtr GetLogger(const std::string& name);

  void OnLogRotate();

 private:
  std::unordered_map<std::string, logging::LoggerPtr> loggers_;
};

}  // namespace components
