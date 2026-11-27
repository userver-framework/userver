#include <userver/engine/wait_any.hpp>

#include <deque>

#include <boost/intrusive/list.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/concurrent/impl/intrusive_mpsc_queue.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/assert.hpp>
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

namespace {

// QueueItems do not need to pass any information through the context parameter of Awaiter:
// a QueueItem itself identifies the subscription.
constexpr std::uintptr_t kUnusedContext = 0;

}  // namespace

// Unlike Impl itself, a QueueItem is an Awaiter, so it has its own wait list hook, allowing
// multiple QueueItems of a single WaitAnyContext to be linked into the WaitList of a single
// shared target (as well as into wait lists of distinct targets).
//
// QueueItems are owned by Impl (queue_items_). The AwaiterPtr ownership passed to wait lists
// is logical: instead of destroying the QueueItem, its dispose hooks release
// the subscription's reference to Impl.
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class WaitAnyContext::QueueItem final
    : public impl::PolymorphicAwaiter,
      public concurrent::impl::SinglyLinkedBaseHook,
      public boost::intrusive::list_base_hook<utils::impl::IntrusiveLinkMode> {
public:
    // State of a QueueItem. Transitions:
    //
    //   kUnused --Append--> kPendingSubscription
    //
    //   kPendingSubscription --TrySubscribe, awaiter appended--------------> kSubscribed
    //   kPendingSubscription --TrySubscribe, already ready (id returned)---> kUnused
    //   kPendingSubscription --Remove--------------------------------------> kUnused
    //
    //   kSubscribed --TryProcessQueue, notification (id returned)----------> kUnused
    //   kSubscribed --Remove, awaiter detached-----------------------------> kUnused
    //   kSubscribed --Remove, notification already in flight---------------> kRemovedWhileNotified
    //
    //   kRemovedWhileNotified --TryProcessQueue, notification (id skipped)--> kUnused
    enum class State : std::uint8_t {
        // The item is free and is stored in unused_.
        kUnused,
        // The item holds an appended awaitable and is stored in pending_subscription_,
        // it is not yet linked to the awaitable.
        kPendingSubscription,
        // The item is linked to the awaitable and waits for its notification.
        kSubscribed,
        // The item was removed by the user, but its notification is already in flight
        // and still has to be popped from notified_.
        kRemovedWhileNotified,
    };

    explicit QueueItem(Impl& owner)
        : owner(owner)
    {}

    void NotifyAndDispose(std::uintptr_t context) noexcept override;

    void DisposeWithoutNotification() noexcept override;

    bool IsLinkedToAwaitable() const noexcept {
        return state == State::kSubscribed || state == State::kRemovedWhileNotified;
    }

    Impl& owner;
    AwaitableToken awaitable;
    std::uint64_t id{0};
    std::uint64_t generation{0};
    State state{State::kUnused};
};

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

    Handle Append(AwaitableToken awaitable);

    Handle Append(std::uint64_t id, engine::AwaitableToken awaitable);

    utils::expected<std::uint64_t, WaitAnyError> WaitUntil(Deadline deadline);

    void Remove(Handle handle) noexcept;

    std::size_t GetSize() const noexcept { return GetWaitedCount() + pending_subscription_.size(); }

    std::uint64_t GetNextId() const noexcept { return next_id_; }

    void Unsubscribe() noexcept;

    void PushNotified(QueueItem& item) noexcept;

private:
    using State = QueueItem::State;

    using Queue = concurrent::impl::IntrusiveMpscQueue<QueueItem>;

    std::optional<std::uint64_t> TryProcessQueue() noexcept;

    std::optional<std::uint64_t> TrySubscribe();

    void RemoveSubscribed(QueueItem& item) noexcept;

    void ProcessNotified(QueueItem& item) noexcept;

    void Recycle(QueueItem& item) noexcept;

    std::size_t GetWaitedCount() const noexcept { return linked_count_ - removed_while_notified_count_; }

    std::uint64_t next_id_{0};
    std::size_t linked_count_{0};
    std::size_t removed_while_notified_count_{0};
    boost::intrusive::list<QueueItem, boost::intrusive::constant_time_size<true>> pending_subscription_;
    boost::intrusive::list<QueueItem, boost::intrusive::constant_time_size<false>> unused_;
    std::deque<QueueItem> queue_items_;
    Queue notified_;
    engine::SingleConsumerEvent queue_non_empty_{engine::SingleConsumerEvent::NoAutoReset{}};
};

WaitAnyContext::Handle WaitAnyContext::Impl::Append(AwaitableToken awaitable) {
    const auto handle = Append(next_id_, awaitable);
    ++next_id_;
    return handle;
}

WaitAnyContext::Handle WaitAnyContext::Impl::Append(std::uint64_t id, engine::AwaitableToken awaitable) {
    if (awaitable.IsEmpty()) {
        return Handle{};
    }
    if (unused_.empty()) {
        queue_items_.emplace_back(*this);
        unused_.push_front(queue_items_.back());
    }
    auto& item = unused_.front();
    unused_.pop_front();
    UASSERT(item.state == State::kUnused);
    item.awaitable = awaitable;
    item.id = id;
    item.state = State::kPendingSubscription;
    pending_subscription_.push_front(item);
    return Handle{&item, item.generation};
}

utils::expected<std::uint64_t, WaitAnyError> WaitAnyContext::Impl::WaitUntil(Deadline deadline) {
    for (;;) {
        queue_non_empty_.Reset();
        if (linked_count_ != 0) {
            auto result = TryProcessQueue();
            if (result.has_value()) {
                return *result;
            }
        }

        auto result = TrySubscribe();
        if (result.has_value()) {
            return *result;
        }

        if (GetWaitedCount() == 0) {
            return utils::unexpected(WaitAnyError::kEmpty);
        }

        switch (queue_non_empty_.WaitUntil(deadline)) {
            case FutureStatus::kReady:
                break;
            case FutureStatus::kTimeout:
                return utils::unexpected(WaitAnyError::kTimeout);
            case FutureStatus::kCancelled:
                return utils::unexpected(WaitAnyError::kCancelled);
        }
    }
}

void WaitAnyContext::Impl::Remove(Handle handle) noexcept {
    if (handle.item_ == nullptr) {
        return;
    }
    auto& item = *handle.item_;
    UASSERT_MSG(&item.owner == this, "The handle belongs to another WaitAnyContext");
    if (item.generation != handle.generation_) {
        utils::AbortWithStacktrace("The awaitable has already been reported by Wait* or removed from WaitAnyContext");
    }

    switch (item.state) {
        case State::kPendingSubscription:
            pending_subscription_.erase(pending_subscription_.iterator_to(item));
            Recycle(item);
            return;
        case State::kSubscribed:
            RemoveSubscribed(item);
            return;
        case State::kUnused:
        case State::kRemovedWhileNotified:
            UASSERT_MSG(false, "Items in this state must have another generation");
            return;
    }
}

void WaitAnyContext::Impl::RemoveSubscribed(QueueItem& item) noexcept {
    auto& awaitable = item.awaitable.GetAwaitable(utils::impl::InternalTag{});
    auto removed = awaitable.RemoveAwaiter(item, kUnusedContext);
    if (removed == nullptr) {
        item.state = State::kRemovedWhileNotified;
        ++item.generation;
        ++removed_while_notified_count_;
        return;
    }
    UASSERT(removed.get() == &item);
    removed.reset();
    --linked_count_;
    Recycle(item);
}

void WaitAnyContext::Impl::Unsubscribe() noexcept {
    pending_subscription_.clear();
    unused_.clear();

    if (linked_count_ == 0) {
        return;
    }
    for (auto& item : queue_items_) {
        if (!item.IsLinkedToAwaitable()) {
            continue;
        }
        if (item.state == State::kSubscribed) {
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
        }
        if (--linked_count_ == 0) {
            break;
        }
    }
}

std::optional<std::uint64_t> WaitAnyContext::Impl::TryProcessQueue() noexcept {
    for (;;) {
        QueueItem* const item = notified_.TryPopBlocking();
        if (item == nullptr) {
            return std::nullopt;
        }

        const bool is_removed = item->state == State::kRemovedWhileNotified;
        ProcessNotified(*item);
        if (!is_removed) {
            return item->id;
        }
    }
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
                    Recycle(item);
                    // Dropping awaiter_ptr calls DisposeWithoutNotification, which releases
                    // the subscription's reference to Impl.
                    awaiter_ptr.reset();
                } else {
                    item.state = State::kSubscribed;
                    ++linked_count_;
                }
            });

            awaitable.TryAppendAwaiter(awaiter_ptr, kUnusedContext);
        }

        if (item.state != State::kSubscribed) {  // awaitable is already ready.
            return item.id;
        }
    }
    return std::nullopt;
}

void WaitAnyContext::Impl::ProcessNotified(QueueItem& item) noexcept {
    UASSERT(item.IsLinkedToAwaitable());
    if (item.state == State::kRemovedWhileNotified) {
        --removed_while_notified_count_;
    } else {
        UASSERT(item.awaitable.GetAwaitable(utils::impl::InternalTag{}).IsReady());
    }

    --linked_count_;
    Recycle(item);
}

void WaitAnyContext::Impl::PushNotified(QueueItem& item) noexcept {
    notified_.Push(item);
    queue_non_empty_.Send();
}

void WaitAnyContext::Impl::Recycle(QueueItem& item) noexcept {
    item.state = State::kUnused;
    ++item.generation;
    unused_.push_front(item);
}

void WaitAnyContext::QueueItem::NotifyAndDispose(std::uintptr_t context) noexcept {
    UASSERT(context == kUnusedContext);

    Impl& owner_snapshot = owner;
    // After Push, `*this` may be immediately recycled and reused by the waiter,
    // so `this` must not be accessed, only Impl.
    owner_snapshot.PushNotified(*this);
    // Release the subscription's reference to Impl, see above. After this point,
    // Impl may be destroyed (if WaitAnyContext is destroyed concurrently).
    intrusive_ptr_release(&owner_snapshot);
}

void WaitAnyContext::QueueItem::DisposeWithoutNotification() noexcept {
    // The QueueItem is removed from a wait list without notification, or the subscription
    // did not happen. Release the subscription's reference to Impl.
    intrusive_ptr_release(&owner);
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

utils::expected<std::uint64_t, WaitAnyError> WaitAnyContext::Wait() { return WaitUntil(Deadline{}); }

utils::expected<std::uint64_t, WaitAnyError> WaitAnyContext::WaitUntil(Deadline deadline) {
    UASSERT(impl_ != nullptr);
    return impl_->WaitUntil(deadline);
}

void WaitAnyContext::Remove(Handle handle) noexcept {
    UASSERT(impl_ != nullptr);
    impl_->Remove(handle);
}

std::size_t WaitAnyContext::GetSize() const noexcept {
    UASSERT(impl_ != nullptr);
    return impl_->GetSize();
}

std::uint64_t WaitAnyContext::GetNextId() const noexcept {
    UASSERT(impl_ != nullptr);
    return impl_->GetNextId();
}

WaitAnyContext::Handle WaitAnyContext::AppendToken(engine::AwaitableToken awaitable) {
    UASSERT(impl_ != nullptr);
    return impl_->Append(awaitable);
}

WaitAnyContext::Handle WaitAnyContext::AppendToken(std::uint64_t id, engine::AwaitableToken awaitable) {
    UASSERT(impl_ != nullptr);
    return impl_->Append(id, awaitable);
}

}  // namespace engine

USERVER_NAMESPACE_END
