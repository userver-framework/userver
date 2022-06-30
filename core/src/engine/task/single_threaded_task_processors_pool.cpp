#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

std::shared_ptr<engine::impl::TaskProcessorPools> GetCurrentEvPool() {
  return engine::current_task::GetTaskProcessor().GetTaskProcessorPools();
}

}  // namespace

SingleThreadedTaskProcessorsPool::SingleThreadedTaskProcessorsPool(
    const engine::TaskProcessorConfig& config_base) {
  auto libev_pool = GetCurrentEvPool();
  const auto task_processors_count = config_base.worker_threads;

  auto config = config_base;
  config.worker_threads = 1;
  if (config.name.empty()) {
    config.name = "single-threaded";
  }
  config.name += '-';
  config.thread_name += '-';

  processors_.reserve(task_processors_count);
  for (size_t i = 0; i < task_processors_count; ++i) {
    auto proc_config = config;
    proc_config.name += std::to_string(i);
    proc_config.thread_name += std::to_string(i);
    processors_.push_back(std::make_unique<engine::TaskProcessor>(
        std::move(proc_config), libev_pool));
  }
}

SingleThreadedTaskProcessorsPool::~SingleThreadedTaskProcessorsPool() = default;

SingleThreadedTaskProcessorsPool SingleThreadedTaskProcessorsPool::MakeForTests(
    std::size_t worker_threads) {
  TaskProcessorConfig config;
  config.name = "test";
  config.worker_threads = worker_threads;

  return SingleThreadedTaskProcessorsPool{config};
}

}  // namespace engine

USERVER_NAMESPACE_END
