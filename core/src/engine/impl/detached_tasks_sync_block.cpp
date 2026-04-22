#include <userver/engine/impl/detached_tasks_sync_block.hpp>

#include <atomic>
#include <optional>

#include <engine/task/task_base_impl.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/not_null.hpp>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct DetachedTasksSyncBlock::Token final : public PolymorphicAwaiter {
    explicit Token(DetachedTasksSyncBlock& owner)
        : PolymorphicAwaiter(Awaiter::kOne),
          owner(owner)
    {}

    void DoNotify(boost::intrusive_ptr<PolymorphicAwaiter> self, std::uintptr_t context) noexcept override {
        UASSERT(context == 0);

        UASSERT(self->UseCount() == 1);
        [[maybe_unused]] auto* detached_awaiter = self.detach();

        DetachedTasksSyncBlock::Dispose(*this);
    }

    void Destroy() noexcept override {
        utils::AbortWithStacktrace("DetachedTasksSyncBlock::Token should never be removed without notification");
    }

    concurrent::impl::IntrusiveWalkablePoolHook<Token> pool_hook{};

    utils::NotNull<DetachedTasksSyncBlock*> owner;

    // For cancellations
    std::atomic<TaskContext*> task{nullptr};

    // For waiting for a cancelled task
    utils::impl::WaitTokenStorageLock wait_token{};
};

// Token is non-standard-layout type, because it is a polymorphic class.
// GCC complains that compilers are not required to implement offsetof for such types.
// TODO Find some way to work around that, e.g. split Token into multiple types?
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

struct DetachedTasksSyncBlock::Impl final {
    std::optional<utils::impl::WaitTokenStorage> wait_tokens{};
    concurrent::impl::IntrusiveWalkablePool<  //
        Token,
        concurrent::impl::MemberHook<&Token::pool_hook>,
        offsetof(Token, pool_hook)>
        cancel_tokens{};
    std::atomic<TaskCancellationReason> cancel_new_tasks{TaskCancellationReason::kNone};
};

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

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

    boost::intrusive_ptr<Awaiter> awaiter{&token, /*add_ref=*/false};
    context.TryAppendAwaiter(awaiter, 0);
    if (awaiter != nullptr) {  // task has already finished.
        impl::Notify(std::move(awaiter), 0);
    }

    const auto cancel_reason = impl_->cancel_new_tasks.load();
    if (cancel_reason != TaskCancellationReason::kNone) {
        context.RequestCancel(cancel_reason);
    }
}

void DetachedTasksSyncBlock::Add(Task&& task) {
    const auto context = std::move(task.pimpl_->context);
    Add(*context);
}

void DetachedTasksSyncBlock::Dispose(Token& token) noexcept {
    auto* const context_ptr = token.task.exchange(nullptr);
    if (context_ptr != nullptr) {
        const boost::intrusive_ptr<TaskContext> context(
            context_ptr,
            /*add_ref=*/false
        );
    }
    [[maybe_unused]] const auto wait_token = std::move(token.wait_token);
    token.owner->impl_->cancel_tokens.Release(token);
}

void DetachedTasksSyncBlock::RequestCancellation(TaskCancellationReason reason) noexcept {
    impl_->cancel_new_tasks.store(reason);

    impl_->cancel_tokens.Walk([&](Token& token) {
        auto* const context_ptr = token.task.exchange(nullptr);

        if (context_ptr != nullptr) {
            const boost::intrusive_ptr<TaskContext> context(
                context_ptr,
                /*add_ref=*/false
            );
            context->RequestCancel(reason);
        }
    });

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
