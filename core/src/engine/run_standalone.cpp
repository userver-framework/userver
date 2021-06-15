#include <engine/run_standalone.hpp>

#include <engine/standalone.hpp>
#include <engine/task/task_context.hpp>
#include <utils/assert.hpp>

namespace engine {

void RunStandalone(std::function<void()> payload) {
  RunStandalone(1, std::move(payload));
}

void RunStandalone(std::size_t worker_threads, std::function<void()> payload) {
  YTX_INVARIANT(!engine::current_task::GetCurrentTaskContextUnchecked(),
                "RunStandalone must not be used alongside a running engine");
  YTX_INVARIANT(worker_threads != 0, "Unable to run anything using 0 threads");

  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          worker_threads, "coro-runner",
          engine::impl::MakeTaskProcessorPools({10, 100, {}, {}}));

  engine::impl::RunOnTaskProcessorSync(*task_processor_holder,
                                       std::move(payload));
}

}  // namespace engine
