#include <mutex>
#include <stdexcept>

#include <engine/standalone.hpp>
#include <engine/task/task_context.hpp>
#include <utils/assert.hpp>

void RunInCoro(std::function<void()> user_cb, size_t worker_threads,
               std::optional<size_t> initial_coro_pool_size,
               std::optional<size_t> max_coro_pool_size) {
  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    UASSERT_MSG(false, "RunInCoro must not be used alongside a running engine");
    throw std::logic_error(
        "RunInCoro must not be used alongside a running engine");
  }

  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          worker_threads, "coro-runner",
          engine::impl::MakeTaskProcessorPools(
              {initial_coro_pool_size.value_or(10),
               max_coro_pool_size.value_or(100),
               {},
               {}}));

  engine::impl::RunOnTaskProcessorSync(*task_processor_holder, user_cb);
}
