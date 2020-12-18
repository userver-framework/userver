#pragma once

#include <components/loggable_component_base.hpp>
#include <engine/task/single_threaded_task_processors_pool.hpp>
#include <engine/task/task_processor_fwd.hpp>

#include <memory>
#include <vector>

namespace engine {
struct TaskProcessorConfig;
}  // namespace engine

namespace components {

class SingleThreadedTaskProcessors final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "single-threaded-task-processors";

  SingleThreadedTaskProcessors(const ComponentConfig& config,
                               const ComponentContext&);

  ~SingleThreadedTaskProcessors() override;

  engine::SingleThreadedTaskProcessorsPool& GetPool() { return pool_; }

 private:
  engine::SingleThreadedTaskProcessorsPool pool_;
};

}  // namespace components
