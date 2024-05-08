#pragma once

/// @file userver/storages/rocks/component.hpp
/// @brief @copybrief rocks::Rocks

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class Rocks : public LoggableComponentBase {
 public:
  // Component constructor
  Rocks(const ComponentConfig&, const ComponentContext&);

  // Component destructor
  ~Rocks() = default;

  // make Client
  template <typename Client>
  Client MakeClient(const std::string& db_path);

  // TODO: implement static yaml_config::Schema GetStaticConfigSchema();

 private:
  engine::TaskProcessor& task_processor_;
};

}  // namespace components

USERVER_NAMESPACE_END
