#include <userver/engine/impl/detached_tasks_sync_block.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include <boost/intrusive_ptr.hpp>

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

#include <concurrent/impl/locked_list.hpp>
#include <concurrent/impl/sharded_pointer_array.hpp>
#include <engine/impl/task_context_accessor.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

struct Token final : public PolymorphicAwaiter {
    Token(concurrent::impl::ShardedPointerArray& shards, std::optional<utils::impl::WaitTokenStorage>& wait_tokens)
        : shard(shards[shards.GetCurrentShardId()]),
          wait_token(wait_tokens ? wait_tokens->GetToken() : utils::impl::WaitTokenStorageLock{})
    {}

    TaskCancellationReason RegisterIfNotCancelled(
        TaskContext& context,
        const std::atomic<TaskCancellationReason>& cancel_new_tasks
    ) noexcept;

    void NotifyAndDispose(std::uintptr_t context) noexcept override;

    void DisposeWithoutNotification() noexcept override {
        utils::AbortWithStacktrace("DetachedTasksSyncBlock::Token should never be removed without notification");
    }

    std::intptr_t& shard;
    utils::impl::WaitTokenStorageLock wait_token;
    // Non-null while linked. The caller keeps the context alive during Add, and the worker
    // holds a reference through NotifyAndDispose. Access to the pointer and hook requires the shard lock.
    TaskContext* task{nullptr};
    concurrent::impl::DoublyLinkedHook<Token> hook;
};

using LockedTokenList = concurrent::impl::LockedList<Token, concurrent::impl::MemberHook<&Token::hook>>;

TaskCancellationReason Token::RegisterIfNotCancelled(
    TaskContext& context,
    const std::atomic<TaskCancellationReason>& cancel_new_tasks
) noexcept {
    LockedTokenList locked{shard};
    // - If we insert this token first, `RequestCancellation` will find it under the shard lock
    //   and cancel the task unless it has already finished.
    // - If `RequestCancellation` locks the shard first, its release on unlock and our acquire on lock
    //   make its store to `cancel_new_tasks` visible here. `Add` requests cancellation with the returned reason.
    const auto reason = cancel_new_tasks.load(std::memory_order_relaxed);
    if (reason == TaskCancellationReason::kNone) {
        task = &context;
        locked.GetList().PushFront(*this);
    }
    return reason;
}

void Token::NotifyAndDispose(std::uintptr_t context) noexcept {
    UASSERT(context == 0);
    // The wait token must outlive the guard: releasing it can let the waiter destroy the shards.
    const std::unique_ptr<Token> self{this};
    LockedTokenList locked{shard};
    if (task) {
        locked.GetList().Erase(*this);
    }
}

}  // namespace

struct DetachedTasksSyncBlock::Impl final {
    std::optional<utils::impl::WaitTokenStorage> wait_tokens;
    concurrent::impl::ShardedPointerArray shards;
    std::atomic<TaskCancellationReason> cancel_new_tasks{TaskCancellationReason::kNone};
};

DetachedTasksSyncBlock::DetachedTasksSyncBlock(StopMode stop_mode) {
    if (stop_mode == StopMode::kCancelAndWait) {
        impl_->wait_tokens.emplace();
    }
}

DetachedTasksSyncBlock::~DetachedTasksSyncBlock() = default;

void DetachedTasksSyncBlock::Add(TaskContext& context) {
    auto token = std::make_unique<Token>(impl_->shards, impl_->wait_tokens);
    const auto cancel_reason = token->RegisterIfNotCancelled(context, impl_->cancel_new_tasks);
    AwaiterPtr awaiter{token.release()};
    context.TryAppendAwaiter(awaiter, 0);
    if (awaiter != nullptr) {  // task has already finished.
        impl::NotifyAndDispose(std::move(awaiter), 0);
    }

    if (cancel_reason != TaskCancellationReason::kNone) {
        context.RequestCancel(cancel_reason);
    }
}

void DetachedTasksSyncBlock::Add(Task& task) {
    Add(TaskContextAccessor::GetContextRef(task));
    // Invalidate only after successful registration so that `task` stays valid if Add throws.
    [[maybe_unused]] const auto extracted = TaskContextAccessor::ExtractContext(std::move(task));
}

void DetachedTasksSyncBlock::RequestCancellation(TaskCancellationReason reason) noexcept {
    impl_->cancel_new_tasks.store(reason, std::memory_order_relaxed);

    for (auto& shard : impl_->shards.Elements()) {
        for (;;) {
            boost::intrusive_ptr<TaskContext> task;
            {
                LockedTokenList locked{shard};
                auto& list = locked.GetList();
                if (list.IsEmpty()) {
                    break;
                }
                auto& token = list.PopFront();
                task.reset(token.task);
                token.task = nullptr;
            }
            task->RequestCancel(reason);
        }
    }

    if (impl_->wait_tokens) {
        impl_->wait_tokens->WaitForAllTokens();
    }
}

void DetachedTasksSyncBlock::WaitAllTasksCompleteDebug() noexcept {
    if (impl_->wait_tokens) {
        impl_->wait_tokens->WaitForAllTokens();
    }
}

std::int64_t DetachedTasksSyncBlock::ActiveTasksApprox() const noexcept {
    UASSERT_MSG(impl_->wait_tokens, "Task count is only available for StopMode::kCancelAndWait");
    if (!impl_->wait_tokens) {
        return 0;
    }

    return impl_->wait_tokens->AliveTokensApprox();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
