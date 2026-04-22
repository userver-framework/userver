#include <userver/engine/wait_any.hpp>

#include <deque>

#include <boost/intrusive/slist.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/concurrent/impl/intrusive_mpsc_queue.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
std::optional<std::size_t> DoWaitAny(utils::span<ContextAccessor*> targets, Deadline deadline) {
    UASSERT_MSG(AreUniqueValues(targets), "Same tasks/futures were detected in WaitAny* call");
    bool none_valid = true;

    for (const auto& [idx, target] : utils::enumerate(targets)) {
        if (!target) {
            continue;
        }
        none_valid = false;
        if (target->IsReady()) {
            return idx;
        }
    }

    if (none_valid) {
        return std::nullopt;
    }

    auto& current = current_task::GetCurrentTaskContext();
    WaitAnyWaitStrategy wait_strategy{targets, current};
    current.Sleep(wait_strategy, deadline);

    for (const auto& [idx, target] : utils::enumerate(targets)) {
        if (target && target->IsReady()) {
            return idx;
        }
    }

    return std::nullopt;
}

}  // namespace impl

class WaitAnyContext::Impl final : public impl::PolymorphicAwaiter {
public:
    Impl();
    ~Impl() noexcept = default;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    void Append(impl::ContextAccessor* awaitable);

    std::optional<std::uint64_t> WaitUntil(Deadline deadline);

    std::size_t GetSize() const noexcept { return subscribed_count_ + pending_subscription_.size(); }

    std::uint64_t GetNextIndex() const noexcept { return next_index_; }

    void Unsubscribe() noexcept;

private:
    // NOLINTNEXTLINE(fuchsia-multiple-inheritance)
    struct QueueItem
        : public concurrent::impl::SinglyLinkedBaseHook,
          public boost::intrusive::slist_base_hook<utils::impl::IntrusiveLinkMode> {
        explicit QueueItem(impl::ContextAccessor* context_accessor, std::uint64_t index)
            : context_accessor(context_accessor),
              index(index)
        {}

        impl::ContextAccessor* context_accessor;
        std::uint64_t index;
        bool subscribed{false};
    };

    using Queue = concurrent::impl::IntrusiveMpscQueue<QueueItem>;

    void DoNotify(boost::intrusive_ptr<impl::PolymorphicAwaiter> self, std::uintptr_t context) noexcept override;

    void Destroy() noexcept override { delete this; }

    std::optional<std::uint64_t> TryProcessQueue() noexcept;

    std::optional<std::uint64_t> TrySubscribe();

    bool ProcessNotified(QueueItem& item) noexcept;

    std::uint64_t next_index_{0};
    std::size_t subscribed_count_{0};
    boost::intrusive::slist<QueueItem, boost::intrusive::constant_time_size<true>> pending_subscription_;
    boost::intrusive::slist<QueueItem, boost::intrusive::constant_time_size<false>> unused_;
    std::deque<QueueItem> queue_items_;
    Queue notified_;
    engine::SingleConsumerEvent queue_non_empty_{engine::SingleConsumerEvent::NoAutoReset{}};
};

WaitAnyContext::Impl::Impl()
    : PolymorphicAwaiter(Awaiter::InitialRefCounter::kOne)
{}

void WaitAnyContext::Impl::Append(impl::ContextAccessor* awaitable) {
    if (awaitable == nullptr) {
        ++next_index_;
        return;
    }
    if (unused_.empty()) {
        auto& item = queue_items_.emplace_back(awaitable, next_index_++);
        pending_subscription_.push_front(item);
        return;
    }
    auto& item = unused_.front();
    unused_.pop_front();
    item.context_accessor = awaitable;
    item.index = next_index_++;
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
        UASSERT(item.context_accessor != nullptr);
        item.context_accessor->RemoveAwaiter(*this, reinterpret_cast<std::uintptr_t>(&item));
        if (--subscribed_count_ == 0) {
            break;
        }
    }
}

void WaitAnyContext::Impl::DoNotify(boost::intrusive_ptr<impl::PolymorphicAwaiter> /*self*/, std::uintptr_t context)
    noexcept {
    notified_.Push(*reinterpret_cast<QueueItem*>(context));
    queue_non_empty_.Send();
}

std::optional<std::uint64_t> WaitAnyContext::Impl::TryProcessQueue() noexcept {
    for (;;) {
        QueueItem* const item = notified_.TryPopBlocking();
        if (item == nullptr) {
            return std::nullopt;
        }

        if (ProcessNotified(*item)) {
            return item->index;
        }
    }
}

std::optional<std::uint64_t> WaitAnyContext::Impl::TrySubscribe() {
    while (!pending_subscription_.empty()) {
        auto& item = pending_subscription_.front();
        pending_subscription_.pop_front();

        UASSERT(item.context_accessor);

        boost::intrusive_ptr<impl::Awaiter> awaiter_ptr{this};
        item.context_accessor->TryAppendAwaiter(awaiter_ptr, reinterpret_cast<std::uintptr_t>(&item));
        if (awaiter_ptr != nullptr) {  // context_accessor is already ready.
            unused_.push_front(item);
            return item.index;
        }

        item.subscribed = true;
        ++subscribed_count_;
    }
    return std::nullopt;
}

bool WaitAnyContext::Impl::ProcessNotified(QueueItem& item) noexcept {
    UASSERT(item.index < next_index_);
    UASSERT(item.context_accessor);
    UASSERT(item.subscribed);

    item.subscribed = false;
    --subscribed_count_;
    unused_.push_front(item);
    // The caller might have already changed the awaitable's state between Wait* calls.
    // So, we check the ready flag to avoid erroneous wakeups on non-ready awaitables.
    return item.context_accessor->IsReady();
}

WaitAnyContext::WaitAnyContext()
    : impl_(new WaitAnyContext::Impl(), /*add_ref=*/false)
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

std::uint64_t WaitAnyContext::GetNextIndex() const noexcept {
    UASSERT(impl_ != nullptr);
    return impl_->GetNextIndex();
}

void WaitAnyContext::AppendAccessor(impl::ContextAccessor* awaitable) {
    UASSERT(impl_ != nullptr);
    impl_->Append(awaitable);
}

}  // namespace engine

USERVER_NAMESPACE_END
