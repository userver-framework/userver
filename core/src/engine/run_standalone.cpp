#include <userver/engine/run_standalone.hpp>

#include <userver/utils/assert.hpp>

#include <engine/impl/standalone.hpp>
#include <engine/task/task_context.hpp>

namespace engine {

void RunStandalone(std::function<void()> payload) {
  RunStandalone(1, std::move(payload));
}

void RunStandalone(std::size_t worker_threads, std::function<void()> payload) {
  engine::RunStandalone(TaskProcessorPoolsConfig{worker_threads},
                        std::move(payload));
}

void RunStandalone(const TaskProcessorPoolsConfig& config,
                   std::function<void()> payload) {
  YTX_INVARIANT(!engine::current_task::GetCurrentTaskContextUnchecked(),
                "RunStandalone must not be used alongside a running engine");
  YTX_INVARIANT(config.worker_threads != 0,
                "Unable to run anything using 0 threads");

  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          config.worker_threads, "coro-runner",
          engine::impl::MakeTaskProcessorPools(config));

  engine::impl::RunOnTaskProcessorSync(*task_processor_holder,
                                       std::move(payload));
}

}  // namespace engine
