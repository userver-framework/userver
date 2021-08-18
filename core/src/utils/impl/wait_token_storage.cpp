#include <userver/utils/impl/wait_token_storage.hpp>

#include <algorithm>

#include <userver/utils/assert.hpp>

#include <engine/task/task_context.hpp>

namespace utils::impl {

void WaitTokenStorage::TokenDeleter::operator()(
    WaitTokenStorage* storage) noexcept {
  if (--storage->tokens_ == 0) storage->event_.Send();
}

WaitTokenStorage::WaitTokenStorage() = default;

WaitTokenStorage::Token WaitTokenStorage::GetToken() {
  [[maybe_unused]] const auto old_count = tokens_++;
  UASSERT_MSG(old_count > 0, "WaitForAllTokens has already been called");
  return WaitTokenStorage::Token{this};
}

std::int64_t WaitTokenStorage::AliveTokensApprox() const { return tokens_ - 1; }

void WaitTokenStorage::WaitForAllTokens() noexcept {
  if (--tokens_ == 0) event_.Send();

  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    event_.WaitNonCancellable();
  } else {
    // RCU is deleted outside of coroutine, this might be a static variable.
    // Just die and do not wait.
  }
}

}  // namespace utils::impl
