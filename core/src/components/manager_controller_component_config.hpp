#pragma once

#include <unordered_map>

#include <engine/task/task_processor_config.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

struct ManagerControllerDynamicConfig {
  explicit ManagerControllerDynamicConfig(
      const dynamic_config::DocsMap& docs_map);

  formats::json::Value tp_doc;
  engine::TaskProcessorSettings default_settings;
  std::unordered_map<std::string, engine::TaskProcessorSettings> settings;
};

}  // namespace components

USERVER_NAMESPACE_END
