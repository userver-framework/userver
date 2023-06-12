#include <userver/engine/single_use_event.hpp>

#include <userver/engine/task/cancel.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace {

constexpr std::uintptr_t kNotSignaledState{0};
constexpr std::uintptr_t kSignaledState{1};

void WakeUpWaiter(impl::TaskContext& context) noexcept {
  context.Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                 impl::TaskContext::NoEpoch{});
  intrusive_ptr_release(&context);
}

class SingleUseEventWaitStrategy final : public impl::WaitStrategy {
 public:
  SingleUseEventWaitStrategy(std::atomic<std::uintptr_t>& state,
                             impl::TaskContext& current)
      : WaitStrategy({}), state_(state), current_(current) {}

  void SetupWakeups() override {
    UASSERT_MSG(state_ == kNotSignaledState || state_ == kSignaledState,
                "Someone else is waiting on the same SingleUseEvent");

    auto expected = kNotSignaledState;
    if (state_.compare_exchange_strong(
            expected, reinterpret_cast<std::uintptr_t>(&current_))) {
      // not signaled yet, keep sleeping
      return;
    }

    // already signaled, wake up
    UASSERT(state_ == kSignaledState);
    WakeUpWaiter(current_);
  }

  void DisableWakeups() override {}

 private:
  std::atomic<std::uintptr_t>& state_;
  impl::TaskContext& current_;
};

}  // namespace

SingleUseEvent::~SingleUseEvent() {
  UASSERT_MSG(state_ == kNotSignaledState || state_ == kSignaledState,
              "Someone is waiting on a SingleUseEvent that is being destroyed");
}

void SingleUseEvent::WaitNonCancellable() noexcept {
  UASSERT_MSG(state_ == kNotSignaledState || state_ == kSignaledState,
              "Someone else is waiting on the same SingleUseEvent");

  if (state_ == kSignaledState) return;  // optimistic path

  impl::TaskContext& current = current_task::GetCurrentTaskContext();
  intrusive_ptr_add_ref(&current);
  SingleUseEventWaitStrategy wait_manager(state_, current);

  engine::TaskCancellationBlocker cancel_blocker;
  [[maybe_unused]] const auto wakeup_source = current.Sleep(wait_manager);
  UASSERT(wakeup_source == impl::TaskContext::WakeupSource::kWaitList);
}

void SingleUseEvent::Send() noexcept {
  auto waiter = state_.exchange(kSignaledState);
  UASSERT_MSG(waiter != kSignaledState,
              "Send called twice in the same SingleUseEvent waiting session");

  if (waiter != kNotSignaledState) {
    WakeUpWaiter(*reinterpret_cast<impl::TaskContext*>(waiter));
  }
}

void SingleUseEvent::Reset() noexcept {
  UASSERT_MSG(state_ == kSignaledState,
              "Reset must not be called in the middle of a waiting session");
  state_ = kNotSignaledState;
}

bool SingleUseEvent::IsReady() const noexcept {
  return state_ == kSignaledState;
}

}  // namespace engine

USERVER_NAMESPACE_END
