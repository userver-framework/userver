#include <userver/engine/io/fd_poller.hpp>

#include <engine/ev/watcher.hpp>
#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light_with_epoch.hpp>
#include <engine/task/task_context.hpp>

template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::io::FdPoller::State> {
    static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename State, typename FormatContext>
    auto format(State state, FormatContext& ctx) const {
        std::string_view str = "broken";
        switch (state) {
            case State::kInvalid:
                str = "invalid";
                break;
            case State::kReadyToUse:
                str = "ready to use";
                break;
            case State::kInUse:
                str = "in use";
                break;
        }

        return fmt::format_to(ctx.out(), "{}", str);
    }
};

USERVER_NAMESPACE_BEGIN

namespace engine::io {

namespace {

int GetEvMode(FdPoller::Kind kind) {
    switch (kind) {
        case FdPoller::Kind::kRead:
            return EV_READ;
        case FdPoller::Kind::kWrite:
            return EV_WRITE;
        case FdPoller::Kind::kReadWrite:
            return EV_READ | EV_WRITE;

        default:
            UINVARIANT(false, "Invalid kind: " + std::to_string(static_cast<int>(kind)));
    }
}

FdPoller::Kind GetUserMode(int ev_events) {
    UASSERT((ev_events & ~(EV_READ | EV_WRITE)) == 0);

    if ((ev_events & EV_READ) && (ev_events & EV_WRITE)) {
        return FdPoller::Kind::kReadWrite;
    }

    if (ev_events & EV_READ) {
        return FdPoller::Kind::kRead;
    }

    if (ev_events & EV_WRITE) {
        return FdPoller::Kind::kWrite;
    }

    UINVARIANT(false, "Failed to recognize events that happened on the socket.");
}

}  // namespace

class FdPoller::Impl final : public engine::impl::ContextAccessor {
public:
    Impl(ev::ThreadControl control);

    ~Impl();

    std::optional<FdPoller::Kind> Wait(Deadline deadline);

    bool IsValid() const noexcept;

    int GetFd() const noexcept { return watcher_.GetFd(); }

    void Invalidate();

    void Reset(int fd, Kind kind);

    bool DoGetAndResetReady() noexcept {
        UASSERT(IsValid());
        // Increment epoch for the new FdPoller usage.
        // This invalidates any pending notifications from the previous use.
        return awaiters_.SetEpochThenGetAndResetSignal(static_cast<ev::WatcherEpoch>(awaiters_.GetEpoch() + 1));
    }

    void ResetReady() noexcept { DoGetAndResetReady(); }

    std::optional<FdPoller::Kind> GetReady() noexcept {
        if (DoGetAndResetReady()) {
            // Cannot read directly from watcher.events, because it's not atomic, and it can potentially
            // be overwritten by another event.
            return events_that_happened_.load(std::memory_order_relaxed);
        } else {
            return std::nullopt;
        }
    }

    void SwitchState(State expected_state, State new_state) noexcept {
        auto old_state = expected_state;
        const auto res = state_.compare_exchange_strong(old_state, new_state);
        UASSERT_MSG(
            res,
            fmt::format("Socket misuse: expected socket state is '{}', actual state is '{}'", expected_state, old_state)
        );
    }

    // ContextAccessor implementation
    bool IsReady() const noexcept override { return awaiters_.IsSignaled(); }

    void TryAppendAwaiter(boost::intrusive_ptr<engine::impl::Awaiter>& awaiter, std::uintptr_t context) override {
        awaiters_.GetSignalOrAppend(awaiter, context);
        if (awaiter == nullptr) {  // Not signaled yet, awaiter was appended.
            watcher_.StartAsync(awaiters_.GetEpoch());
        }
    }

    void RemoveAwaiter(engine::impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        awaiters_.Remove(awaiter, context);
        // we need to stop watcher manually to avoid racy wakeups later
        watcher_.StopAsync();
    }

private:
    static void IoWatcherCb(struct ev_loop*, ev_io*, int) noexcept;

    engine::impl::WaitListLightWithEpoch awaiters_;
    std::atomic<FdPoller::State> state_{FdPoller::State::kInvalid};
    std::atomic<FdPoller::Kind> events_that_happened_{};
    // `watcher` must be the last field to make sure that it is stopped and destroyed first.
    ev::Watcher<ev_io> watcher_;
};

FdPoller::Impl::Impl(ev::ThreadControl control)
    : awaiters_(0),
      watcher_(control, this)
{
    static_assert(decltype(state_)::is_always_lock_free);
    static_assert(decltype(events_that_happened_)::is_always_lock_free);
    watcher_.Init(&IoWatcherCb);
}

FdPoller::Impl::~Impl() = default;

std::optional<FdPoller::Kind> FdPoller::Impl::Wait(Deadline deadline) {
    UASSERT(IsValid());

    auto& current = current_task::GetCurrentTaskContext();

    engine::impl::FutureWaitStrategy wait_strategy{*this, current};
    current.Sleep(wait_strategy, deadline);

    // Don't call heavy synchronous Stop() here. The epoch system will handle stale notifications.
    return GetReady();
}

void FdPoller::Impl::Invalidate() {
    UASSERT(IsValid());
    // Synchronously stop the watcher to avoid spurious wakeups and to ensure that the fd is not used anymore.
    watcher_.Stop();
    awaiters_.GetAndResetSignal();

    auto old_state = State::kReadyToUse;
    const auto res = state_.compare_exchange_strong(old_state, State::kInvalid);

    UINVARIANT(
        res,
        fmt::format("Socket misuse: expected socket state is '{}', actual state is '{}'", State::kReadyToUse, old_state)
    );
}

void FdPoller::Impl::Reset(int fd, Kind kind) {
    UASSERT(!IsValid());
    UASSERT(watcher_.GetFd() == fd || watcher_.GetFd() == -1);
    watcher_.Set(fd, GetEvMode(kind));
    state_ = State::kReadyToUse;
}

void FdPoller::Impl::IoWatcherCb(struct ev_loop*, ev_io* watcher, int) noexcept {
    const auto ev_events = watcher->events;

    UASSERT(watcher->active);

    auto* self = static_cast<FdPoller::Impl*>(watcher->data);

    // Cleanup watcher_ first, then awake the coroutine.
    // Otherwise, the coroutine may close watcher_'s fd before watcher_ is stopped.
    const auto guard = self->watcher_.StopWithinEvCallback();

    self->events_that_happened_.store(GetUserMode(ev_events), std::memory_order_relaxed);
    const auto cb_epoch = self->watcher_.GetEpochWithinEvCallback();
    self->awaiters_.SetSignalAndNotifyOneIfEpochMatches(cb_epoch);
}

bool FdPoller::Impl::IsValid() const noexcept { return state_ != State::kInvalid; }

FdPoller::FdPoller(const ev::ThreadControl& control)
    : pimpl_(control)
{
    static_assert(std::atomic<State>::is_always_lock_free);
}

FdPoller::~FdPoller() = default;

FdPoller::operator bool() const noexcept { return IsValid(); }

bool FdPoller::IsValid() const noexcept { return pimpl_->IsValid(); }

int FdPoller::GetFd() const noexcept { return pimpl_->GetFd(); }

std::optional<FdPoller::Kind> FdPoller::Wait(Deadline deadline) { return pimpl_->Wait(deadline); }

void FdPoller::ResetReady() noexcept { pimpl_->ResetReady(); }

std::optional<FdPoller::Kind> FdPoller::GetReady() noexcept { return pimpl_->GetReady(); }

engine::impl::ContextAccessor* FdPoller::TryGetContextAccessor() noexcept { return &*pimpl_; }

void FdPoller::Reset(int fd, Kind kind) { pimpl_->Reset(fd, kind); }

void FdPoller::Invalidate() { pimpl_->Invalidate(); }

void FdPoller::SwitchStateToInUse() { pimpl_->SwitchState(State::kReadyToUse, State::kInUse); }

void FdPoller::SwitchStateToReadyToUse() { pimpl_->SwitchState(State::kInUse, State::kReadyToUse); }

}  // namespace engine::io

USERVER_NAMESPACE_END
