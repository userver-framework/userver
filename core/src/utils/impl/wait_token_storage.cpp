#include <userver/utils/impl/wait_token_storage.hpp>

#include <utility>

#include <userver/utils/assert.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

WaitTokenStorage::Token::Token(WaitTokenStorage& storage) noexcept
    : storage_(&storage) {
  [[maybe_unused]] const auto old_count = storage_->tokens_++;
  UASSERT_MSG(old_count > 0, "WaitForAllTokens has already been called");
}

WaitTokenStorage::Token::Token(Token&& other) noexcept
    : storage_(std::exchange(other.storage_, nullptr)) {}

WaitTokenStorage::Token::Token(const Token& other) noexcept
    : storage_(other.storage_) {
  if (storage_ != nullptr) storage_->tokens_++;
}

WaitTokenStorage::Token& WaitTokenStorage::Token::operator=(
    Token&& other) noexcept {
  if (&other != this) {
    [[maybe_unused]] const Token for_disposal{std::move(*this)};
    storage_ = std::exchange(other.storage_, nullptr);
  }
  return *this;
}

WaitTokenStorage::Token& WaitTokenStorage::Token::operator=(
    const Token& other) noexcept {
  *this = Token{other};
  return *this;
}

WaitTokenStorage::Token::~Token() {
  if (storage_ != nullptr) {
    if (--storage_->tokens_ == 0) storage_->event_.Send();
  }
}

WaitTokenStorage::WaitTokenStorage() = default;

WaitTokenStorage::Token WaitTokenStorage::GetToken() { return Token{*this}; }

std::int64_t WaitTokenStorage::AliveTokensApprox() const { return tokens_ - 1; }

void WaitTokenStorage::WaitForAllTokens() noexcept {
  if (--tokens_ == 0) event_.Send();

  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    event_.WaitNonCancellable();
  } else {
    // RCU is being destroyed outside of coroutine context. In this case, we
    // have already waited for all async tasks when exiting the coroutine
    // context, and new ones couldn't have been launched.
  }
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
