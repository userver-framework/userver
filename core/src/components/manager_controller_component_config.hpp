#pragma once

#include <engine/task/task_processor.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/taxi_config/value.hpp>

namespace components {

struct ManagerControllerTaxiConfig {
  explicit ManagerControllerTaxiConfig(const taxi_config::DocsMap& docs_map);

  formats::json::Value tp_doc;
  engine::TaskProcessorSettings default_settings;
  std::unordered_map<std::string, engine::TaskProcessorSettings> settings;
};

}  // namespace components
