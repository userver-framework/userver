#include <userver/engine/run_standalone.hpp>

#include <engine/impl/standalone.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

void RunStandalone(std::function<void()> payload) {
  RunStandalone(1, TaskProcessorPoolsConfig{}, std::move(payload));
}

void RunStandalone(std::size_t worker_threads, std::function<void()> payload) {
  RunStandalone(worker_threads, TaskProcessorPoolsConfig{}, std::move(payload));
}

void RunStandalone(std::size_t worker_threads,
                   const TaskProcessorPoolsConfig& config,
                   std::function<void()> payload) {
  UINVARIANT(!engine::current_task::IsTaskProcessorThread(),
             "RunStandalone must not be used alongside a running engine");
  UINVARIANT(worker_threads != 0, "Unable to run anything using 0 threads");

  auto task_processor_holder = engine::impl::TaskProcessorHolder::Make(
      worker_threads, "coro-runner",
      engine::impl::MakeTaskProcessorPools(config));

  engine::impl::RunOnTaskProcessorSync(*task_processor_holder,
                                       std::move(payload));
}

}  // namespace engine

USERVER_NAMESPACE_END
