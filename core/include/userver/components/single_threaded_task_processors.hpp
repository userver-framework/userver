#pragma once

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
/// The component does **not** have any options for service config.
class SingleThreadedTaskProcessors final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "single-threaded-task-processors";

  SingleThreadedTaskProcessors(const ComponentConfig& config,
                               const ComponentContext&);

  ~SingleThreadedTaskProcessors() override;

  engine::SingleThreadedTaskProcessorsPool& GetPool() { return pool_; }

  static std::string GetStaticConfigSchema();

 private:
  engine::SingleThreadedTaskProcessorsPool pool_;
};

template <>
inline constexpr bool kHasValidate<SingleThreadedTaskProcessors> = true;

}  // namespace components

USERVER_NAMESPACE_END
