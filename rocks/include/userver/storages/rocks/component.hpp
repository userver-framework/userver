#pragma once

/// @file userver/storages/rocks/component.hpp
/// @brief @copybrief rocks::Rocks

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/client_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

// clang-format off

/// @ingroup userver_components
///
/// @brief RocksDB client component.
/// ## Static options:
/// Name                               | Description                                      | Default value
/// ---------------------------------- | ------------------------------------------------ | ---------------
/// task-processor                     | name of the task processor to run the blocking file operations | -
/// db-path                            | path to database file                            | -

// clang-format on

class Component : public components::LoggableComponentBase {
 public:
  Component(const components::ComponentConfig&,
            const components::ComponentContext&);

  ~Component() = default;

  storages::rocks::ClientPtr MakeClient();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::rocks::ClientPtr client_ptr_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
