#include <userver/engine/wait_all_checked.hpp>

#include <limits>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

void RethrowError(AwaitableToken target) {
    auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});

    auto exception = awaitable.GetErrorResult();
    if (exception) {
        std::rethrow_exception(exception);
    }
}

}  // namespace

class WaitAllCheckedContext final {
public:
    explicit WaitAllCheckedContext(std::vector<AwaitableToken>&& targets)
        : impl_(utils::impl::MakeIntrusiveOne<Impl>(std::move(targets)))
    {}

    WaitAllCheckedContext(const WaitAllCheckedContext&) = delete;
    WaitAllCheckedContext(WaitAllCheckedContext&&) = delete;
    WaitAllCheckedContext& operator=(WaitAllCheckedContext&&) = delete;
    WaitAllCheckedContext& operator=(const WaitAllCheckedContext&) = delete;

    ~WaitAllCheckedContext() noexcept { impl_->Unsubscribe(); }

    FutureStatus WaitUntil(Deadline deadline) { return impl_->WaitUntil(deadline); }

private:
    // NOLINTNEXTLINE(fuchsia-multiple-inheritance)
    class Impl final : public impl::PolymorphicAwaiter, public utils::impl::IntrusiveRefCounterOne<Impl> {
    public:
        explicit Impl(std::vector<AwaitableToken>&& targets);

        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        ~Impl() noexcept = default;

        FutureStatus WaitUntil(Deadline deadline);

        void Unsubscribe() noexcept;

    private:
        static constexpr std::size_t kNoExceptionSource = std::numeric_limits<std::size_t>::max();

        AwaiterPtr AsAwaiterPtr() noexcept;

        void NotifyAndDispose(std::uintptr_t context) noexcept override;

        void DisposeWithoutNotification() noexcept override { intrusive_ptr_release(this); }

        std::vector<AwaitableToken> targets_;
        std::size_t next_subscription_index_{0};
        std::atomic<std::size_t> pending_notifications_{0};
        std::atomic<std::size_t> exception_source_index_{kNoExceptionSource};
        engine::SingleConsumerEvent result_ready_;
    };

    boost::intrusive_ptr<Impl> impl_;
};

WaitAllCheckedContext::Impl::Impl(std::vector<AwaitableToken>&& targets)
    : targets_(std::move(targets))
{}

FutureStatus WaitAllCheckedContext::Impl::WaitUntil(Deadline deadline) {
    while (next_subscription_index_ < targets_.size()) {
        const auto target = targets_[next_subscription_index_];
        if (target.IsEmpty()) {
            ++next_subscription_index_;
            continue;
        }
        auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});

        pending_notifications_.fetch_add(1, std::memory_order_relaxed);
        auto awaiter_ptr = AsAwaiterPtr();
        awaitable.TryAppendAwaiter(awaiter_ptr, next_subscription_index_++);
        if (awaiter_ptr != nullptr) {  // target is already ready.
            pending_notifications_.fetch_sub(1, std::memory_order_relaxed);
            RethrowError(target);
        }
    }

    if (!result_ready_.WaitUntil(deadline, [this]() {
            return exception_source_index_.load(std::memory_order_relaxed) != kNoExceptionSource ||
                   pending_notifications_.load(std::memory_order_relaxed) == 0;
        }))
    {
        return current_task::ShouldCancel() ? FutureStatus::kCancelled : FutureStatus::kTimeout;
    }
    auto exception_source_index = exception_source_index_.exchange(kNoExceptionSource, std::memory_order_relaxed);
    if (exception_source_index != kNoExceptionSource) {
        RethrowError(targets_[exception_source_index]);
        utils::AbortWithStacktrace("Should never be there");
    }
    UASSERT(pending_notifications_.load(std::memory_order_relaxed) == 0);
    return FutureStatus::kReady;
}

void WaitAllCheckedContext::Impl::Unsubscribe() noexcept {
    for (std::size_t i = 0; i < next_subscription_index_; ++i) {
        if (targets_[i].IsEmpty()) {
            continue;
        }
        auto& awaitable = targets_[i].GetAwaitable(utils::impl::InternalTag{});
        awaitable.RemoveAwaiter(*this, i);
    }
}

AwaiterPtr WaitAllCheckedContext::Impl::AsAwaiterPtr() noexcept {
    intrusive_ptr_add_ref(this);
    return AwaiterPtr{this};
}

void WaitAllCheckedContext::Impl::NotifyAndDispose(std::uintptr_t context) noexcept {
    UASSERT(context <= std::numeric_limits<std::size_t>::max());
    const auto index = static_cast<std::size_t>(context);
    const auto target = targets_[index];
    auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});
    UASSERT(awaitable.IsReady());
    UASSERT(pending_notifications_.load(std::memory_order_relaxed) > 0);

    bool ready = false;
    if (awaitable.GetErrorResult()) {
        auto expected = kNoExceptionSource;
        if (exception_source_index_.compare_exchange_strong(expected, index, std::memory_order_relaxed)) {
            ready = true;
        }
    }
    if (pending_notifications_.fetch_sub(1, std::memory_order_release) == 1) {
        ready = true;
    }
    if (ready) {
        result_ready_.Send();
    }

    intrusive_ptr_release(this);
}

FutureStatus DoWaitAllChecked(std::vector<AwaitableToken>&& targets, Deadline deadline) {
    UASSERT_MSG(AreUniqueValues(utils::span{targets}), "Same tasks/futures were detected in WaitAny* call");
    WaitAllCheckedContext context{std::move(targets)};
    return context.WaitUntil(deadline);
}

void HandleWaitAllStatus(FutureStatus status) {
    switch (status) {
        case FutureStatus::kReady:
            break;
        case FutureStatus::kTimeout:
            UASSERT_MSG(false, "Got timeout on a WaitAll without Deadline");
            break;
        case FutureStatus::kCancelled:
            throw WaitInterruptedException(current_task::GetCurrentTaskContext().CancellationReason());
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
