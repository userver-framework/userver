#include <userver/engine/wait_all_checked.hpp>

#include <limits>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/fixed_array.hpp>
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
    // The refcounter of Impl counts:
    // 1. the reference from WaitAllCheckedContext (impl_);
    // 2. one reference per subscribed Node. This way, Impl stays alive until all in-flight
    //    notifications complete, even if WaitAllCheckedContext is destroyed concurrently with them.
    class Impl final : public utils::impl::IntrusiveRefCounterOne<Impl> {
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
        // Unlike Impl itself, a Node is an Awaiter, so it has its own wait list hook, allowing
        // multiple Nodes of a single WaitAllCheckedContext to be linked into the WaitList
        // of a single shared target (as well as into wait lists of distinct targets).
        //
        // Nodes are owned by Impl (nodes_). The AwaiterPtr ownership passed to wait lists
        // is logical: instead of destroying the Node, its dispose hooks release
        // the subscription's reference to Impl.
        struct Node final : public impl::PolymorphicAwaiter {
            Node(Impl& owner, AwaitableToken target, std::size_t index)
                : owner(owner),
                  target(target),
                  index(index)
            {}

            void NotifyAndDispose(std::uintptr_t context) noexcept override {
                UASSERT(context == kUnusedContext);

                owner.HandleNotification(index);
                // Release the subscription's reference to Impl, see above. After this point,
                // Impl (including `*this`) may be destroyed (if WaitAllCheckedContext
                // is destroyed concurrently).
                intrusive_ptr_release(&owner);
            }

            void DisposeWithoutNotification() noexcept override {
                // The Node is removed from a wait list without notification. Release
                // the subscription's reference to Impl.
                intrusive_ptr_release(&owner);
            }

            Impl& owner;
            AwaitableToken target;
            std::size_t index;
            bool subscribed{false};
        };

        // Nodes do not need to pass any information through the context parameter of Awaiter:
        // a Node itself identifies the subscription.
        static constexpr std::uintptr_t kUnusedContext = 0;
        static constexpr std::size_t kNoExceptionSource = std::numeric_limits<std::size_t>::max();

        void HandleNotification(std::size_t index) noexcept;

        // One node per target; nodes of empty targets stay unused.
        utils::FixedArray<Node> nodes_;
        std::size_t next_subscription_index_{0};
        std::atomic<std::size_t> pending_notifications_{0};
        std::atomic<std::size_t> exception_source_index_{kNoExceptionSource};
        engine::SingleConsumerEvent result_ready_;
    };

    boost::intrusive_ptr<Impl> impl_;
};

WaitAllCheckedContext::Impl::Impl(std::vector<AwaitableToken>&& targets)
    : nodes_(utils::GenerateFixedArray(
          targets.size(),
          [&](std::size_t index) { return Node(*this, targets[index], index); }
      ))
{}

FutureStatus WaitAllCheckedContext::Impl::WaitUntil(Deadline deadline) {
    while (next_subscription_index_ < nodes_.size()) {
        const auto index = next_subscription_index_++;
        auto& node = nodes_[index];
        const auto target = node.target;
        if (target.IsEmpty()) {
            continue;
        }
        auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});

        pending_notifications_.fetch_add(1, std::memory_order_relaxed);
        // Acquire the subscription's reference to Impl in advance.
        intrusive_ptr_add_ref(this);

        AwaiterPtr awaiter_ptr{&node};

        {
            const utils::FastScopeGuard finalize_subscription([&]() noexcept {
                if (awaiter_ptr != nullptr) {
                    // The target is already ready: deliver the missed notification in-place.
                    // If the target has completed with an exception, it will be rethrown
                    // after the subscription loop.
                    UASSERT(awaiter_ptr.get() == &node);
                    impl::NotifyAndDispose(std::move(awaiter_ptr), kUnusedContext);
                } else {
                    node.subscribed = true;
                }
            });

            awaitable.TryAppendAwaiter(awaiter_ptr, kUnusedContext);
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
        RethrowError(nodes_[exception_source_index].target);
        utils::AbortWithStacktrace("Should never be there");
    }
    UASSERT(pending_notifications_.load(std::memory_order_relaxed) == 0);
    return FutureStatus::kReady;
}

void WaitAllCheckedContext::Impl::Unsubscribe() noexcept {
    for (std::size_t i = 0; i < next_subscription_index_; ++i) {
        auto& node = nodes_[i];
        if (!node.subscribed) {
            continue;
        }
        auto& awaitable = node.target.GetAwaitable(utils::impl::InternalTag{});
        auto removed = awaitable.RemoveAwaiter(node, kUnusedContext);
        if (removed != nullptr) {
            UASSERT(removed.get() == &node);
        }
        // Dropping `removed` calls DisposeWithoutNotification, releasing the subscription's
        // reference to Impl. This never destroys Impl here, because the caller
        // (WaitAllCheckedContext) still holds its own reference. If the node was already
        // notified, `removed` is null, and the in-flight NotifyAndDispose releases
        // the subscription's reference.
        removed.reset();
        node.subscribed = false;
    }
}

void WaitAllCheckedContext::Impl::HandleNotification(std::size_t index) noexcept {
    const auto target = nodes_[index].target;
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
