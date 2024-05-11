#pragma once

/// @file userver/storages/rocks/component.hpp
/// @brief @copybrief rocks::Rocks

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/client_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class Rocks : public LoggableComponentBase {
 public:
  Rocks(const ComponentConfig&, const ComponentContext&);

  ~Rocks() = default;

  storages::rocks::ClientPtr MakeClient();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::rocks::ClientPtr client_ptr_;
};

}  // namespace components

USERVER_NAMESPACE_END
