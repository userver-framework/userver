#include <engine/sleep.hpp>

#include <engine/task/cancel.hpp>

#include <engine/task/task_context.hpp>

namespace engine {

namespace impl {
namespace {
class CommonSleepWaitStrategy final : public WaitStrategy {
 public:
  CommonSleepWaitStrategy(Deadline deadline) : WaitStrategy(deadline) {}

  void AfterAsleep() override {}

  void BeforeAwake() override {}
};
}  // namespace
}  // namespace impl

void InterruptibleSleepUntil(Deadline deadline) {
  auto current = current_task::GetCurrentTaskContext();

  impl::CommonSleepWaitStrategy wait_manager(deadline);
  current->Sleep(wait_manager);
}

void SleepUntil(Deadline deadline) {
  TaskCancellationBlocker block_cancel;
  InterruptibleSleepUntil(deadline);
}

void Yield() { SleepUntil(Deadline::FromTimePoint(Deadline::kPassed)); }

}  // namespace engine
