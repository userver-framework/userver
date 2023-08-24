#include <userver/engine/impl/detached_tasks_sync_block.hpp>

#include <atomic>
#include <optional>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct DetachedTasksSyncBlock::Token final {
  explicit Token(DetachedTasksSyncBlock& owner) : owner(owner) {}

  DetachedTasksSyncBlock& owner;

  concurrent::impl::IntrusiveWalkablePoolHook<Token> pool_hook{};

  // For cancellations
  std::atomic<TaskContext*> task{nullptr};

  // For waiting for a cancelled task
  utils::impl::WaitTokenStorage::Token wait_token{};
};

struct DetachedTasksSyncBlock::Impl final {
  std::optional<utils::impl::WaitTokenStorage> wait_tokens{};
  concurrent::impl::IntrusiveWalkablePool<
      Token, concurrent::impl::MemberHook<&Token::pool_hook>>
      cancel_tokens{};
  std::atomic<TaskCancellationReason> cancel_new_tasks{
      TaskCancellationReason::kNone};
};

DetachedTasksSyncBlock::DetachedTasksSyncBlock(StopMode stop_mode) {
  if (stop_mode == StopMode::kCancelAndWait) {
    impl_->wait_tokens.emplace();
  }
}

DetachedTasksSyncBlock::~DetachedTasksSyncBlock() = default;

void DetachedTasksSyncBlock::Add(TaskContext& context) {
  auto& token = impl_->cancel_tokens.Acquire([this] { return Token(*this); });
  UASSERT(token.task == nullptr);

  boost::intrusive_ptr<TaskContext> context_copy(&context);

  token.task.store(context_copy.detach());
  if (impl_->wait_tokens) {
    token.wait_token = impl_->wait_tokens->GetToken();
  }

  context.SetDetached(token);

  const auto cancel_reason = impl_->cancel_new_tasks.load();
  if (cancel_reason != TaskCancellationReason::kNone) {
    context.RequestCancel(cancel_reason);
  }
}

void DetachedTasksSyncBlock::Add(Task&& task) {
  const auto context = std::move(task.context_);
  Add(*context);
}

void DetachedTasksSyncBlock::Dispose(Token& token) noexcept {
  auto* const context_ptr = token.task.exchange(nullptr);
  if (context_ptr != nullptr) {
    const boost::intrusive_ptr<TaskContext> context(context_ptr,
                                                    /*add_ref=*/false);
  }
  [[maybe_unused]] const auto wait_token = std::move(token.wait_token);
  token.owner.impl_->cancel_tokens.Release(token);
}

void DetachedTasksSyncBlock::RequestCancellation(
    TaskCancellationReason reason) noexcept {
  impl_->cancel_new_tasks.store(reason);

  impl_->cancel_tokens.Walk([&](Token& token) {
    auto* const context_ptr = token.task.exchange(nullptr);

    if (context_ptr != nullptr) {
      boost::intrusive_ptr<TaskContext> context(context_ptr,
                                                /*add_ref=*/false);
      context->RequestCancel(reason);
    }
  });

  if (impl_->wait_tokens) {
    impl_->wait_tokens->WaitForAllTokens();
  }
}

std::int64_t DetachedTasksSyncBlock::ActiveTasksApprox() const noexcept {
  UASSERT_MSG(impl_->wait_tokens,
              "Task count is only available for StopMode::kCancelAndWait");
  if (!impl_->wait_tokens) return 0;

  return impl_->wait_tokens->AliveTokensApprox();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
