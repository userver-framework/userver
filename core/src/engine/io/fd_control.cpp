#include "fd_control.hpp"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <memory>
#include <stdexcept>

#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <utils/check_syscall.hpp>

template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::io::impl::Direction::State> {
  static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::engine::io::impl::Direction::State state,
              FormatContext& ctx) const {
    using State = USERVER_NAMESPACE::engine::io::impl::Direction::State;
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

namespace engine::io::impl {
namespace {

static_assert(std::atomic<Direction::State>::is_always_lock_free);

int SetNonblock(int fd) {
  int oldflags = utils::CheckSyscallCustomException<IoSystemError>(
      ::fcntl(fd, F_GETFL), "getting file status flags, fd={}", fd);
  if (!(oldflags & O_NONBLOCK)) {
    utils::CheckSyscallCustomException<IoSystemError>(
        ::fcntl(fd, F_SETFL, oldflags | O_NONBLOCK),
        "setting file status flags, fd=", fd);
  }
  return fd;
}

int SetCloexec(int fd) {
  int oldflags = utils::CheckSyscallCustomException<IoSystemError>(
      ::fcntl(fd, F_GETFD), "getting file status flags, fd={}", fd);
  if (!(oldflags & FD_CLOEXEC)) {
    utils::CheckSyscallCustomException<IoSystemError>(
        ::fcntl(fd, F_SETFD, oldflags | FD_CLOEXEC),
        "setting file status flags, fd={}", fd);
  }
  return fd;
}

int ReduceSigpipe(int fd) {
#ifdef F_SETNOSIGPIPE
  // may fail for all we care, SIGPIPE is ignored anyway
  ::fcntl(fd, F_SETNOSIGPIPE, 1);
#endif
  return fd;
}

class DirectionWaitStrategy final : public engine::impl::WaitStrategy {
 public:
  DirectionWaitStrategy(Deadline deadline, engine::impl::WaitListLight& waiters,
                        ev::Watcher<ev_io>& watcher,
                        engine::impl::TaskContext& current)
      : WaitStrategy(deadline),
        watcher_(watcher),
        wait_scope_(waiters, current) {}

  void SetupWakeups() override {
    wait_scope_.Append();
    watcher_.StartAsync();
  }

  void DisableWakeups() override {
    wait_scope_.Remove();
    // we need to stop watcher manually to avoid racy wakeups later
    watcher_.StopAsync();
  }

 private:
  ev::Watcher<ev_io>& watcher_;
  engine::impl::WaitScopeLight wait_scope_;
};

}  // namespace

void FdControlDeleter::operator()(FdControl* ptr) const noexcept {
  std::default_delete<FdControl>{}(ptr);
}

#ifndef NDEBUG
Direction::SingleUserGuard::SingleUserGuard(Direction& dir) : dir_(dir) {
  auto old_state = State::kReadyToUse;
  const auto res =
      dir_.state_.compare_exchange_strong(old_state, State::kInUse);

  UASSERT_MSG(
      res,
      fmt::format(
          "Socket missuse: expected socket state is '{}', actual state is '{}'",
          State::kReadyToUse, old_state));
}

Direction::SingleUserGuard::~SingleUserGuard() {
  auto old_state = State::kInUse;
  const auto res =
      dir_.state_.compare_exchange_strong(old_state, State::kReadyToUse);
  UASSERT_MSG(
      res,
      fmt::format(
          "Socket missuse: expected socket state is '{}', actual state is '{}'",
          State::kInUse, old_state));
}
#endif  // #ifndef NDEBUG

Direction::Direction(Kind kind)
    : kind_(kind),
      state_(State::kInvalid),
      watcher_(current_task::GetEventThread(), this) {
  watcher_.Init(&IoWatcherCb);
}

Direction::~Direction() = default;

bool Direction::Wait(Deadline deadline) {
  return DoWait(deadline) == engine::impl::TaskContext::WakeupSource::kWaitList;
}

engine::impl::TaskContext::WakeupSource Direction::DoWait(Deadline deadline) {
  UASSERT(IsValid());

  auto& current = current_task::GetCurrentTaskContext();

  if (current.ShouldCancel()) {
    return engine::impl::TaskContext::WakeupSource::kCancelRequest;
  }

  impl::DirectionWaitStrategy wait_manager(deadline, *waiters_, watcher_,
                                           current);
  return current.Sleep(wait_manager);
}

void Direction::Reset(int fd) {
  UASSERT(!IsValid());
  UASSERT(fd_ == fd || fd_ == -1);
  fd_ = fd;
  watcher_.Set(fd_, kind_ == Kind::kRead ? EV_READ : EV_WRITE);
  state_ = State::kReadyToUse;
}

void Direction::StopWatcher() {
  UASSERT(IsValid());
  watcher_.Stop();
}

void Direction::WakeupWaiters() { waiters_->WakeupOne(); }

void Direction::Invalidate() {
  StopWatcher();

  auto old_state = State::kReadyToUse;
  const auto res = state_.compare_exchange_strong(old_state, State::kInvalid);

  UINVARIANT(
      res,
      fmt::format(
          "Socket missuse: expected socket state is '{}', actual state is '{}'",
          State::kReadyToUse, old_state));
}

void Direction::IoWatcherCb(struct ev_loop*, ev_io* watcher, int) noexcept {
  UASSERT(watcher->active);
  UASSERT((watcher->events & ~(EV_READ | EV_WRITE)) == 0);

  auto* self = static_cast<Direction*>(watcher->data);
  self->WakeupWaiters();

  // Watcher::Stop() from ev loop should execute synchronously w/o waiting.
  //
  // Should be the last call, because after it the destructor of watcher_ is
  // allowed to return from Stop() without waiting (because of the
  // `!pending_async_ops_ && !is_running_`).
  self->watcher_.Stop();
}

FdControl::FdControl()
    : read_(Direction::Kind::kRead), write_(Direction::Kind::kWrite) {}

FdControl::~FdControl() {
  try {
    Close();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception while destructing: " << e;
  }
}

FdControlHolder FdControl::Adopt(int fd) {
  FdControlHolder fd_control{new FdControl()};
  // TODO: add conditional CLOEXEC set
  SetCloexec(fd);
  SetNonblock(fd);
  ReduceSigpipe(fd);
  fd_control->read_.Reset(fd);
  fd_control->write_.Reset(fd);
  return fd_control;
}

void FdControl::Close() {
  if (!IsValid()) return;
  Invalidate();

  const auto fd = Fd();
  if (::close(fd) == -1) {
    const auto error_code = errno;
    std::error_code ec(error_code, std::system_category());
    UASSERT_MSG(!error_code, "Failed to close fd=" + std::to_string(fd));
    LOG_ERROR() << "Cannot close fd " << fd << ": " << ec.message();
  }

  read_.WakeupWaiters();
  write_.WakeupWaiters();
}

void FdControl::Invalidate() {
  read_.Invalidate();
  write_.Invalidate();
}

}  // namespace engine::io::impl

USERVER_NAMESPACE_END
