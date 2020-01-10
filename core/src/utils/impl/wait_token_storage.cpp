#include <utils/impl/wait_token_storage.hpp>

#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>

namespace utils::impl {

class WaitTokenStorage::SafeScopeGuard final {
 public:
  using Callback = std::function<void()>;
  explicit SafeScopeGuard(Callback callback) : callback_(std::move(callback)) {}

  SafeScopeGuard(const SafeScopeGuard&) = delete;
  SafeScopeGuard(SafeScopeGuard&&) = delete;

  SafeScopeGuard& operator=(const SafeScopeGuard&) = delete;
  SafeScopeGuard& operator=(SafeScopeGuard&&) = delete;

  ~SafeScopeGuard() {
    if (!callback_) return;

    callback_();
  }

 private:
  Callback callback_;
};

WaitTokenStorage::WaitTokenStorage()
    : event_(std::make_shared<engine::SingleConsumerEvent>()),
      token_(std::make_shared<SafeScopeGuard>(
          [event = event_] { event->Send(); })) {}

std::shared_ptr<WaitTokenStorage::SafeScopeGuard> WaitTokenStorage::GetToken() {
  return token_;
}

void WaitTokenStorage::WaitForAllTokens() {
  token_.reset();
  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    // Make sure all data is deleted after return from dtr
    engine::TaskCancellationBlocker blocker;
    [[maybe_unused]] bool ok = event_->WaitForEvent();
    UASSERT(ok);
  } else {
    // RCU is deleted outside of coroutine, this might be a static variable.
    // Just die and do not wait.
  }
}

}  // namespace utils::impl
