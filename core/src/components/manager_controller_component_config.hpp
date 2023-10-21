#pragma once

#include <string>
#include <unordered_map>

#include <engine/task/task_processor_config.hpp>
#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

struct ManagerControllerDynamicConfig {
  static ManagerControllerDynamicConfig Parse(
      const dynamic_config::DocsMap& docs_map);

  engine::TaskProcessorSettings default_settings;
  std::unordered_map<std::string, engine::TaskProcessorSettings> settings;
};

extern const dynamic_config::Key<ManagerControllerDynamicConfig>
    kManagerControllerDynamicConfig;

}  // namespace components

USERVER_NAMESPACE_END
