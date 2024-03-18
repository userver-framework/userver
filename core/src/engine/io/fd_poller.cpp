#include <userver/engine/io/fd_poller.hpp>

#include <engine/ev/watcher.hpp>
#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light.hpp>
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
      UINVARIANT(false,
                 "Invalid kind: " + std::to_string(static_cast<int>(kind)));
  }
}

FdPoller::Kind GetUserMode(int ev_events) {
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

struct FdPoller::Impl final : public engine::impl::ContextAccessor {
  Impl();

  ~Impl();

  engine::impl::TaskContext::WakeupSource DoWait(Deadline deadline);

  bool IsValid() const noexcept;

  void Invalidate();
  void Reset(int fd, Kind kind);

  void StopWatcher();

  static void IoWatcherCb(struct ev_loop*, ev_io*, int) noexcept;
  void WakeupWaiters();

  void ResetReady() noexcept { waiters_->GetAndResetSignal(); }

  // ContextAccessor implementation
  bool IsReady() const noexcept override { return waiters_->IsSignaled(); }

  engine::impl::EarlyWakeup TryAppendWaiter(
      engine::impl::TaskContext& waiter) override {
    if (waiters_->GetSignalOrAppend(&waiter)) {
      return engine::impl::EarlyWakeup{true};
    }
    watcher_.StartAsync();
    return engine::impl::EarlyWakeup{false};
  }

  void RemoveWaiter(engine::impl::TaskContext& waiter) noexcept override {
    waiters_->Remove(waiter);
    // we need to stop watcher manually to avoid racy wakeups later
    watcher_.StopAsync();
  }

  void AfterWait() noexcept override { watcher_.Stop(); }

  void RethrowErrorResult() const override {}

  int fd_{-1};
  std::atomic<FdPoller::State> state_{FdPoller::State::kInvalid};
  engine::impl::FastPimplWaitListLight waiters_;
  ev::Watcher<ev_io> watcher_;
  std::atomic<FdPoller::Kind> events_that_happened_{};
};

void FdPoller::Impl::WakeupWaiters() { waiters_->SetSignalAndWakeupOne(); }

FdPoller::Impl::Impl() : watcher_(current_task::GetEventThread(), this) {
  watcher_.Init(&IoWatcherCb);
}

FdPoller::Impl::~Impl() = default;

engine::impl::TaskContext::WakeupSource FdPoller::Impl::DoWait(
    Deadline deadline) {
  UASSERT(IsValid());

  auto& current = current_task::GetCurrentTaskContext();

  engine::impl::FutureWaitStrategy wait_strategy{*this, current};
  auto ret = current.Sleep(wait_strategy, deadline);

  /*
   * Manually call Stop() here to be sure that after DoWait() no waiter_'s
   * callback (IoWatcherCb) is running.
   */
  watcher_.Stop();
  return ret;
}

void FdPoller::Impl::Invalidate() {
  StopWatcher();

  auto old_state = State::kReadyToUse;
  const auto res = state_.compare_exchange_strong(old_state, State::kInvalid);

  UINVARIANT(
      res,
      fmt::format(
          "Socket misuse: expected socket state is '{}', actual state is '{}'",
          State::kReadyToUse, old_state));
}

void FdPoller::Impl::StopWatcher() {
  UASSERT(IsValid());
  watcher_.Stop();
}

void FdPoller::Impl::IoWatcherCb(struct ev_loop*, ev_io* watcher,
                                 int) noexcept {
  const auto ev_events = watcher->events;

  UASSERT(watcher->active);
  UASSERT((ev_events & ~(EV_READ | EV_WRITE)) == 0);

  auto* self = static_cast<FdPoller::Impl*>(watcher->data);

  /* Cleanup watcher_ first, then awake the coroutine.
   * Otherwise, the coroutine may close watcher_'s fd
   * before watcher_ is stopped.
   */
  self->watcher_.Stop();

  self->events_that_happened_.store(GetUserMode(ev_events),
                                    std::memory_order_relaxed);
  self->WakeupWaiters();
}

bool FdPoller::Impl::IsValid() const noexcept {
  return state_ != State::kInvalid;
}

FdPoller::FdPoller() { static_assert(std::atomic<State>::is_always_lock_free); }

FdPoller::~FdPoller() = default;

FdPoller::operator bool() const noexcept { return IsValid(); }

bool FdPoller::IsValid() const noexcept { return pimpl_->IsValid(); }

int FdPoller::GetFd() const { return pimpl_->fd_; }

std::optional<FdPoller::Kind> FdPoller::Wait(Deadline deadline) {
  ResetReady();
  if (pimpl_->DoWait(deadline) ==
      engine::impl::TaskContext::WakeupSource::kWaitList) {
    return pimpl_->events_that_happened_.load(std::memory_order_relaxed);
  } else {
    return std::nullopt;
  }
}

std::optional<FdPoller::Kind> FdPoller::GetReady() noexcept {
  if (pimpl_->waiters_->GetAndResetSignal()) {
    return pimpl_->events_that_happened_.load(std::memory_order_relaxed);
  } else {
    return std::nullopt;
  }
}

engine::impl::ContextAccessor* FdPoller::TryGetContextAccessor() noexcept {
  return &*pimpl_;
}

void FdPoller::Reset(int fd, Kind kind) { pimpl_->Reset(fd, kind); }

void FdPoller::Invalidate() { pimpl_->Invalidate(); }

void FdPoller::WakeupWaiters() { pimpl_->WakeupWaiters(); }

void FdPoller::SwitchStateToInUse() {
  auto old_state = State::kReadyToUse;
  const auto res =
      pimpl_->state_.compare_exchange_strong(old_state, State::kInUse);

  UASSERT_MSG(
      res,
      fmt::format(
          "Socket misuse: expected socket state is '{}', actual state is '{}'",
          State::kReadyToUse, old_state));
}

void FdPoller::SwitchStateToReadyToUse() {
  auto old_state = State::kInUse;
  const auto res =
      pimpl_->state_.compare_exchange_strong(old_state, State::kReadyToUse);
  UASSERT_MSG(
      res,
      fmt::format(
          "Socket misuse: expected socket state is '{}', actual state is '{}'",
          State::kInUse, old_state));
}

void FdPoller::Impl::Reset(int fd, Kind kind) {
  UASSERT(!IsValid());
  UASSERT(fd_ == fd || fd_ == -1);
  fd_ = fd;
  watcher_.Set(fd_, GetEvMode(kind));
  state_ = State::kReadyToUse;
}

void FdPoller::ResetReady() noexcept { pimpl_->ResetReady(); }

}  // namespace engine::io

USERVER_NAMESPACE_END
