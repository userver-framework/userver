#include <userver/utils/impl/wait_token_storage.hpp>

#include <utility>

#include <userver/utils/assert.hpp>

#include <userver/engine/task/task.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

namespace {
constexpr std::int64_t kInitialTokensCount = 1;
}  // namespace

WaitTokenStorage::Token::Token(WaitTokenStorage& storage) noexcept
    : storage_(&storage) {
  [[maybe_unused]] const auto old_count = storage_->tokens_++;
  if (old_count < kInitialTokensCount) {
    AbortWithStacktrace("WaitForAllTokens has already been called");
  }
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

WaitTokenStorage::WaitTokenStorage() : tokens_(kInitialTokensCount) {}

WaitTokenStorage::~WaitTokenStorage() {
  if (tokens_ > kInitialTokensCount) {
    // WaitForAllTokens has not been called (e.g. an exception has been thrown
    // from WaitTokenStorage owner's constructor), and there are some tokens
    // still alive. Don't wait for them, because that can cause a hard-to-detect
    // deadlock.
    AbortWithStacktrace(
        "Some tokens are still alive while the WaitTokenStorage is being "
        "destroyed");
  }
}

WaitTokenStorage::Token WaitTokenStorage::GetToken() { return Token{*this}; }

std::uint64_t WaitTokenStorage::AliveTokensApprox() const noexcept {
  return std::max(tokens_.load() - kInitialTokensCount, std::int64_t{0});
}

void WaitTokenStorage::WaitForAllTokens() noexcept {
  if (tokens_ < kInitialTokensCount) {
    UASSERT_MSG(false, "WaitForAllTokens must be called at most once");
    return;
  }

  if (--tokens_ == 0) event_.Send();

  if (engine::current_task::IsTaskProcessorThread()) {
    event_.WaitNonCancellable();
  } else {
    // RCU is being destroyed outside of coroutine context. In this case, we
    // have already waited for all async tasks when exiting the coroutine
    // context, and new ones couldn't have been launched.
  }
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
