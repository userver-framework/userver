#include <userver/engine/wait_all_checked.hpp>

#include <limits>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

void RethrowError(ContextAccessor& accessor) {
    auto exception = accessor.GetErrorResult();
    if (exception) {
        std::rethrow_exception(exception);
    }
}

}  // namespace

class WaitAllCheckedContext final {
public:
    WaitAllCheckedContext(std::vector<ContextAccessor*>&& targets)
        : impl_(new Impl(std::move(targets)), /*add_ref=*/false)
    {}
    ~WaitAllCheckedContext() noexcept { impl_->Unsubscribe(); }
    WaitAllCheckedContext(const WaitAllCheckedContext&) = delete;
    WaitAllCheckedContext(WaitAllCheckedContext&&) = delete;
    WaitAllCheckedContext& operator=(WaitAllCheckedContext&&) = delete;
    WaitAllCheckedContext& operator=(const WaitAllCheckedContext&) = delete;

    FutureStatus WaitUntil(Deadline deadline) { return impl_->WaitUntil(deadline); }

private:
    class Impl final : public impl::PolymorphicAwaiter {
    public:
        Impl(std::vector<ContextAccessor*>&& targets);
        ~Impl() noexcept = default;

        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        FutureStatus WaitUntil(Deadline deadline);

        void Unsubscribe() noexcept;

    private:
        void DoNotify(boost::intrusive_ptr<impl::PolymorphicAwaiter> self, std::uintptr_t context) noexcept override;

        void Destroy() noexcept override { delete this; }

        std::vector<ContextAccessor*> targets_;
        std::size_t next_subscription_index_{0};
        std::atomic<std::size_t> pending_notifications_{0};
        std::atomic<ContextAccessor*> exception_source_{nullptr};
        engine::SingleConsumerEvent result_ready_;
    };

    boost::intrusive_ptr<Impl> impl_;
};

WaitAllCheckedContext::Impl::Impl(std::vector<ContextAccessor*>&& targets)
    : PolymorphicAwaiter(Awaiter::InitialRefCounter::kOne),
      targets_(std::move(targets))
{}

FutureStatus WaitAllCheckedContext::Impl::WaitUntil(Deadline deadline) {
    while (next_subscription_index_ < targets_.size()) {
        auto target = targets_[next_subscription_index_];
        if (target == nullptr) {
            ++next_subscription_index_;
            continue;
        }

        pending_notifications_.fetch_add(1, std::memory_order_relaxed);
        boost::intrusive_ptr<impl::Awaiter> awaiter_ptr{this};
        target->TryAppendAwaiter(awaiter_ptr, next_subscription_index_++);
        if (awaiter_ptr != nullptr) {  // target is already ready.
            pending_notifications_.fetch_sub(1, std::memory_order_relaxed);
            RethrowError(*target);
        }
    }

    if (!result_ready_.WaitUntil(deadline, [this]() {
            return exception_source_.load(std::memory_order_relaxed) != nullptr ||
                   pending_notifications_.load(std::memory_order_relaxed) == 0;
        }))
    {
        return current_task::ShouldCancel() ? FutureStatus::kCancelled : FutureStatus::kTimeout;
    }
    auto exception_source = exception_source_.exchange(nullptr, std::memory_order_relaxed);
    if (exception_source != nullptr) {
        RethrowError(*exception_source);
        utils::AbortWithStacktrace("Should never be there");
    }
    UASSERT(pending_notifications_.load(std::memory_order_relaxed) == 0);
    return FutureStatus::kReady;
}

void WaitAllCheckedContext::Impl::Unsubscribe() noexcept {
    for (std::size_t i = 0; i < next_subscription_index_; ++i) {
        if (targets_[i] == nullptr) {
            continue;
        }
        targets_[i]->RemoveAwaiter(*this, i);
    }
}

void WaitAllCheckedContext::Impl::DoNotify(
    boost::intrusive_ptr<impl::PolymorphicAwaiter> /*self*/,
    std::uintptr_t context
) noexcept {
    UASSERT(context <= std::numeric_limits<std::size_t>::max());
    const auto index = static_cast<std::size_t>(context);
    UASSERT(targets_[index] != nullptr);
    UASSERT(targets_[index]->IsReady());
    UASSERT(pending_notifications_.load(std::memory_order_relaxed) > 0);

    bool ready = false;
    if (targets_[index]->GetErrorResult()) {
        ContextAccessor* expected{nullptr};
        if (exception_source_.compare_exchange_strong(expected, targets_[index], std::memory_order_relaxed)) {
            ready = true;
        }
    }
    if (pending_notifications_.fetch_sub(1, std::memory_order_release) == 1) {
        ready = true;
    }
    if (ready) {
        result_ready_.Send();
    }
}

FutureStatus DoWaitAllChecked(std::vector<ContextAccessor*>&& targets, Deadline deadline) {
    UASSERT_MSG(AreUniqueValues(targets), "Same tasks/futures were detected in WaitAny* call");
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
