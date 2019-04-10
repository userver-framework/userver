#include <engine/standalone.hpp>

#include <engine/coro/pool_config.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/task_processor_pools.hpp>

namespace engine {
namespace impl {

std::shared_ptr<TaskProcessorPools> MakeTaskProcessorPools(
    const TaskProcessorPoolsConfig& pools_config) {
  coro::PoolConfig coro_config;
  if (pools_config.initial_coro_pool_size)
    coro_config.initial_size = *pools_config.initial_coro_pool_size;
  if (pools_config.max_coro_pool_size)
    coro_config.max_size = *pools_config.max_coro_pool_size;

  ev::ThreadPoolConfig ev_config;
  if (pools_config.ev_threads_num)
    ev_config.threads = *pools_config.ev_threads_num;
  if (pools_config.ev_thread_name)
    ev_config.thread_name = *pools_config.ev_thread_name;

  // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg,clang-analyzer-core.uninitialized.UndefReturn)
  return std::make_shared<TaskProcessorPools>(std::move(coro_config),
                                              std::move(ev_config));
}

TaskProcessorHolder TaskProcessorHolder::MakeTaskProcessor(
    size_t threads_num, std::string thread_name,
    std::shared_ptr<TaskProcessorPools> pools) {
  TaskProcessorConfig config;
  config.worker_threads = threads_num;
  config.thread_name = std::move(thread_name);

  return TaskProcessorHolder(
      std::make_unique<TaskProcessor>(std::move(config), std::move(pools)));
}

TaskProcessorHolder::TaskProcessorHolder(
    std::unique_ptr<TaskProcessor>&& task_processor)
    : task_processor_(std::move(task_processor)) {}

TaskProcessorHolder::~TaskProcessorHolder() = default;

TaskProcessorHolder::TaskProcessorHolder(TaskProcessorHolder&&) noexcept =
    default;

TaskProcessorHolder& TaskProcessorHolder::operator=(
    TaskProcessorHolder&&) noexcept = default;

}  // namespace impl
}  // namespace engine
