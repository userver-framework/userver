#pragma once

/// @file userver/components/single_threaded_task_processors.hpp
/// @brief @copybrief components::SingleThreadedTaskProcessors

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/single_threaded_task_processors_pool.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace engine {
struct TaskProcessorConfig;
}  // namespace engine

namespace components {

/// @ingroup userver_components
///
/// @brief Component that starts multiple single threaded task processors.
///
/// Usefull to process tasks in a single threaded third-party libraries
/// (for example in Python/JS interpreters).
///
/// ## Static options:
/// See "Static task_processor options" at
/// components::ManagerControllerComponent for options description and
/// sample.
class SingleThreadedTaskProcessors final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "single-threaded-task-processors";

  SingleThreadedTaskProcessors(const ComponentConfig& config,
                               const ComponentContext&);

  ~SingleThreadedTaskProcessors() override;

  engine::SingleThreadedTaskProcessorsPool& GetPool() { return pool_; }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  engine::SingleThreadedTaskProcessorsPool pool_;
};

template <>
inline constexpr bool kHasValidate<SingleThreadedTaskProcessors> = true;

}  // namespace components

USERVER_NAMESPACE_END
