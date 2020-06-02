#include "fd_control.hpp"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <memory>
#include <stdexcept>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/check_syscall.hpp>

#include <engine/task/task_context.hpp>
#include <engine/wait_list.hpp>
#include <utils/assert.hpp>

namespace engine::io::impl {
namespace {

int SetNonblock(int fd) {
  int oldflags = utils::CheckSyscall(::fcntl(fd, F_GETFL),
                                     "getting file status flags, fd=", fd);
  if (!(oldflags & O_NONBLOCK)) {
    utils::CheckSyscall(::fcntl(fd, F_SETFL, oldflags | O_NONBLOCK),
                        "setting file status flags, fd=", fd);
  }
  return fd;
}

int SetCloexec(int fd) {
  int oldflags = utils::CheckSyscall(::fcntl(fd, F_GETFD),
                                     "getting file status flags, fd=", fd);
  if (!(oldflags & FD_CLOEXEC)) {
    utils::CheckSyscall(::fcntl(fd, F_SETFD, oldflags | FD_CLOEXEC),
                        "setting file status flags, fd=", fd);
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
  DirectionWaitStrategy(Deadline deadline, engine::impl::WaitList& waiters,
                        ev::Watcher<ev_io>& watcher,
                        engine::impl::TaskContext* current)
      : WaitStrategy(deadline),
        waiters_(waiters),
        lock_(waiters_),
        watcher_(watcher),
        current_(current) {}

  void AfterAsleep() override {
    waiters_.Append(lock_, current_);
    lock_.Release();

    watcher_.StartAsync();
  }

  void BeforeAwake() override {
    // we need to stop watcher manually to avoid racy wakeups later
    engine::impl::WaitList::Lock lock(waiters_);
    if (waiters_.IsEmpty(lock)) {
      // locked queueing to avoid race w/ StartAsync in wait strategy
      watcher_.StopAsync();
    }
  }

  engine::impl::WaitListBase* GetWaitList() override { return &waiters_; }

 private:
  engine::impl::WaitList& waiters_;
  engine::impl::WaitList::Lock lock_;
  ev::Watcher<ev_io>& watcher_;
  engine::impl::TaskContext* const current_;
};

}  // namespace

Direction::Direction(Kind kind)
    : fd_(-1),
      kind_(kind),
      is_valid_(false),
      waiters_(),
      watcher_(current_task::GetEventThread(), this) {
  watcher_.Init(&IoWatcherCb);
}

Direction::~Direction() = default;

bool Direction::Wait(Deadline deadline) {
  UASSERT(IsValid());

  auto current = current_task::GetCurrentTaskContext();

  if (current->ShouldCancel()) return false;

  impl::DirectionWaitStrategy wait_manager(deadline, *waiters_, watcher_,
                                           current);
  current->Sleep(&wait_manager);

  return (current->GetWakeupSource() ==
          engine::impl::TaskContext::WakeupSource::kWaitList);
}

void Direction::Reset(int fd) {
  UASSERT(!IsValid());
  UASSERT(fd_ == fd || fd_ == -1);
  fd_ = fd;
  watcher_.Set(fd_, kind_ == Kind::kRead ? EV_READ : EV_WRITE);
  is_valid_ = true;
}

void Direction::StopWatcher() {
  UASSERT(is_valid_);
  watcher_.Stop();
}

void Direction::WakeupWaiters() {
  engine::impl::WaitList::Lock lock(*waiters_);
  waiters_->WakeupAll(lock);
}

void Direction::Invalidate() {
  StopWatcher();
  is_valid_ = false;
}

// NOLINTNEXTLINE(bugprone-exception-escape)
void Direction::IoWatcherCb(struct ev_loop*, ev_io* watcher, int) noexcept {
  UASSERT(watcher->active);
  UASSERT((watcher->events & ~(EV_READ | EV_WRITE)) == 0);

  auto self = static_cast<Direction*>(watcher->data);
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
  auto fd_control = std::make_shared<FdControl>();
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
