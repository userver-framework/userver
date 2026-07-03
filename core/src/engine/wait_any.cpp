#include <userver/engine/wait_any.hpp>

#include <deque>

#include <boost/intrusive/slist.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/concurrent/impl/intrusive_mpsc_queue.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
std::optional<std::size_t> DoWaitAny(utils::span<AwaitableToken> target_tokens, Deadline deadline) {
    UASSERT_MSG(AreUniqueValues(target_tokens), "Same tasks/futures were detected in WaitAny* call");
    bool none_valid = true;

    for (const auto& [idx, target] : utils::enumerate(target_tokens)) {
        if (target.IsEmpty()) {
            continue;
        }
        const auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});
        none_valid = false;
        if (awaitable.IsReady()) {
            return idx;
        }
    }

    if (none_valid) {
        return std::nullopt;
    }

    auto& current = current_task::GetCurrentTaskContext();
    WaitAnyAwaitable wait_any_awaitable{target_tokens};
    current.Sleep(wait_any_awaitable, deadline);

    for (const auto& [idx, target] : utils::enumerate(target_tokens)) {
        if (target.IsEmpty()) {
            continue;
        }
        const auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});
        if (awaitable.IsReady()) {
            return idx;
        }
    }

    return std::nullopt;
}

}  // namespace impl

// The refcounter of Impl counts:
// 1. the reference from WaitAnyContext (impl_);
// 2. one reference per subscribed QueueItem. This way, Impl stays alive until all in-flight
//    notifications complete, even if WaitAnyContext is destroyed concurrently with them.
class WaitAnyContext::Impl final : public utils::impl::IntrusiveRefCounterOne<Impl> {
public:
    Impl() = default;
    ~Impl() noexcept = default;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    void Append(AwaitableToken awaitable);

    void Append(std::uint64_t id, engine::AwaitableToken awaitable);

    std::optional<std::uint64_t> WaitUntil(Deadline deadline);

    std::size_t GetSize() const noexcept { return subscribed_count_ + pending_subscription_.size(); }

    std::uint64_t GetNextId() const noexcept { return next_id_; }

    void Unsubscribe() noexcept;

private:
    // Unlike Impl itself, a QueueItem is an Awaiter, so it has its own wait list hook, allowing
    // multiple QueueItems of a single WaitAnyContext to be linked into the WaitList of a single
    // shared target (as well as into wait lists of distinct targets).
    //
    // QueueItems are owned by Impl (queue_items_). The AwaiterPtr ownership passed to wait lists
    // is logical: instead of destroying the QueueItem, its dispose hooks release
    // the subscription's reference to Impl.
    // NOLINTNEXTLINE(fuchsia-multiple-inheritance)
    struct QueueItem final
        : public impl::PolymorphicAwaiter,
          public concurrent::impl::SinglyLinkedBaseHook,
          public boost::intrusive::slist_base_hook<utils::impl::IntrusiveLinkMode> {
        explicit QueueItem(Impl& owner, AwaitableToken awaitable, std::uint64_t id)
            : owner(owner),
              awaitable(awaitable),
              id(id)
        {}

        void NotifyAndDispose(std::uintptr_t context) noexcept override {
            UASSERT(context == kUnusedContext);

            Impl& owner_snapshot = owner;
            // After Push, `*this` may be immediately recycled and reused by the waiter,
            // so `this` must not be accessed, only Impl.
            owner_snapshot.notified_.Push(*this);
            owner_snapshot.queue_non_empty_.Send();
            // Release the subscription's reference to Impl, see above. After this point,
            // Impl may be destroyed (if WaitAnyContext is destroyed concurrently).
            intrusive_ptr_release(&owner_snapshot);
        }

        void DisposeWithoutNotification() noexcept override {
            // The QueueItem is removed from a wait list without notification, or the subscription
            // did not happen. Release the subscription's reference to Impl.
            intrusive_ptr_release(&owner);
        }

        Impl& owner;
        AwaitableToken awaitable;
        std::uint64_t id;
        bool subscribed{false};
    };

    using Queue = concurrent::impl::IntrusiveMpscQueue<QueueItem>;

    // QueueItems do not need to pass any information through the context parameter of Awaiter:
    // a QueueItem itself identifies the subscription.
    static constexpr std::uintptr_t kUnusedContext = 0;

    std::optional<std::uint64_t> TryProcessQueue() noexcept;

    std::optional<std::uint64_t> TrySubscribe();

    void ProcessNotified(QueueItem& item) noexcept;

    std::uint64_t next_id_{0};
    std::size_t subscribed_count_{0};
    boost::intrusive::slist<QueueItem, boost::intrusive::constant_time_size<true>> pending_subscription_;
    boost::intrusive::slist<QueueItem, boost::intrusive::constant_time_size<false>> unused_;
    std::deque<QueueItem> queue_items_;
    Queue notified_;
    engine::SingleConsumerEvent queue_non_empty_{engine::SingleConsumerEvent::NoAutoReset{}};
};

void WaitAnyContext::Impl::Append(AwaitableToken awaitable) {
    Append(next_id_, awaitable);
    ++next_id_;
}

void WaitAnyContext::Impl::Append(std::uint64_t id, engine::AwaitableToken awaitable) {
    if (awaitable.IsEmpty()) {
        return;
    }
    if (unused_.empty()) {
        auto& item = queue_items_.emplace_back(*this, awaitable, id);
        pending_subscription_.push_front(item);
        return;
    }
    auto& item = unused_.front();
    unused_.pop_front();
    item.awaitable = awaitable;
    item.id = id;
    pending_subscription_.push_front(item);
}

std::optional<std::uint64_t> WaitAnyContext::Impl::WaitUntil(Deadline deadline) {
    for (;;) {
        queue_non_empty_.Reset();
        if (subscribed_count_ != 0) {
            auto result = TryProcessQueue();
            if (result.has_value()) {
                return result;
            }
        }

        auto result = TrySubscribe();
        if (result.has_value()) {
            return result;
        }

        if (subscribed_count_ == 0) {
            return std::nullopt;
        }

        if (!queue_non_empty_.WaitForEventUntil(deadline)) {
            return std::nullopt;
        }
    }
}

void WaitAnyContext::Impl::Unsubscribe() noexcept {
    pending_subscription_.clear();
    unused_.clear();

    if (subscribed_count_ == 0) {
        return;
    }
    for (auto& item : queue_items_) {
        if (!item.subscribed) {
            continue;
        }
        auto& awaitable = item.awaitable.GetAwaitable(utils::impl::InternalTag{});
        auto removed = awaitable.RemoveAwaiter(item, kUnusedContext);
        if (removed != nullptr) {
            UASSERT(removed.get() == &item);
        }
        // Dropping `removed` calls DisposeWithoutNotification, releasing the subscription's
        // reference to Impl. This never destroys Impl here, because the caller (WaitAnyContext)
        // still holds its own reference. If the item was already notified, `removed` is null,
        // and the in-flight NotifyAndDispose releases the subscription's reference.
        removed.reset();
        if (--subscribed_count_ == 0) {
            break;
        }
    }
}

std::optional<std::uint64_t> WaitAnyContext::Impl::TryProcessQueue() noexcept {
    QueueItem* const item = notified_.TryPopBlocking();
    if (item == nullptr) {
        return std::nullopt;
    }

    ProcessNotified(*item);
    return item->id;
}

std::optional<std::uint64_t> WaitAnyContext::Impl::TrySubscribe() {
    while (!pending_subscription_.empty()) {
        auto& item = pending_subscription_.front();
        pending_subscription_.pop_front();

        auto& awaitable = item.awaitable.GetAwaitable(utils::impl::InternalTag{});

        // Acquire the subscription's reference to Impl in advance.
        intrusive_ptr_add_ref(this);

        impl::AwaiterPtr awaiter_ptr{&item};

        {
            const utils::FastScopeGuard finalize_subscription([&]() noexcept {
                if (awaiter_ptr != nullptr) {
                    // The awaitable is already ready, or TryAppendAwaiter has thrown:
                    // the subscription did not happen.
                    UASSERT(awaiter_ptr.get() == &item);
                    unused_.push_front(item);
                    // Dropping awaiter_ptr calls DisposeWithoutNotification, which releases
                    // the subscription's reference to Impl.
                    awaiter_ptr.reset();
                } else {
                    item.subscribed = true;
                    ++subscribed_count_;
                }
            });

            awaitable.TryAppendAwaiter(awaiter_ptr, kUnusedContext);
        }

        if (!item.subscribed) {  // awaitable is already ready.
            return item.id;
        }
    }
    return std::nullopt;
}

void WaitAnyContext::Impl::ProcessNotified(QueueItem& item) noexcept {
    UASSERT(item.subscribed);
    UASSERT(item.awaitable.GetAwaitable(utils::impl::InternalTag{}).IsReady());

    item.subscribed = false;
    --subscribed_count_;
    unused_.push_front(item);
}

WaitAnyContext::WaitAnyContext()
    : impl_(utils::impl::MakeIntrusiveOne<WaitAnyContext::Impl>())
{}

WaitAnyContext::~WaitAnyContext() noexcept {
    if (impl_ == nullptr) {
        // Instance has been moved out.
        return;
    }
    impl_->Unsubscribe();
}

WaitAnyContext::WaitAnyContext(WaitAnyContext&&) noexcept = default;

WaitAnyContext& WaitAnyContext::operator=(WaitAnyContext&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    WaitAnyContext cleanup{std::move(*this)};
    impl_ = std::move(other.impl_);
    return *this;
}

std::optional<std::uint64_t> WaitAnyContext::Wait() { return WaitUntil(Deadline{}); }

std::optional<std::uint64_t> WaitAnyContext::WaitUntil(Deadline deadline) {
    UASSERT(impl_ != nullptr);
    return impl_->WaitUntil(deadline);
}

std::size_t WaitAnyContext::GetSize() const noexcept {
    UASSERT(impl_ != nullptr);
    return impl_->GetSize();
}

std::uint64_t WaitAnyContext::GetNextId() const noexcept {
    UASSERT(impl_ != nullptr);
    return impl_->GetNextId();
}

void WaitAnyContext::AppendToken(engine::AwaitableToken awaitable) {
    UASSERT(impl_ != nullptr);
    impl_->Append(awaitable);
}

void WaitAnyContext::AppendToken(std::uint64_t id, engine::AwaitableToken awaitable) {
    UASSERT(impl_ != nullptr);
    impl_->Append(id, awaitable);
}

}  // namespace engine

USERVER_NAMESPACE_END
