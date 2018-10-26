#include "fd_control.hpp"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cassert>
#include <memory>
#include <stdexcept>

#include <logging/log.hpp>
#include <utils/check_syscall.hpp>

namespace engine {
namespace io {
namespace impl {
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

int GetNofileLimit() {
  static constexpr int kDefaultNofileLimit = 4 * 65536;  // ought to be enough

  struct rlimit rlimit_nofile;
  utils::CheckSyscall(::getrlimit(RLIMIT_NOFILE, &rlimit_nofile),
                      "getting open files limits");

  if (rlimit_nofile.rlim_max != RLIM_INFINITY) {
    return rlimit_nofile.rlim_max;
  }
  return kDefaultNofileLimit;
}

}  // namespace

Direction::Direction(Kind kind)
    : fd_(-1),
      kind_(kind),
      is_valid_(false),
      waiters_(std::make_shared<engine::impl::WaitList>()),
      watcher_(current_task::GetEventThread(), this) {
  watcher_.Init(&IoWatcherCb);
}

void Direction::Wait(Deadline deadline) {
  assert(IsValid());
  engine::impl::WaitList::Lock lock(*waiters_);

  auto caller_ctx = current_task::GetCurrentTaskContext();
  engine::impl::TaskContext::SleepParams sleep_params;
  sleep_params.deadline = std::move(deadline);
  sleep_params.wait_list = waiters_;
  sleep_params.exec_after_asleep = [this, &lock, caller_ctx] {
    waiters_->Append(lock, caller_ctx);
    lock.Release();
    watcher_.StartAsync();
  };

  caller_ctx->Sleep(std::move(sleep_params));
}

void Direction::Reset(int fd) {
  assert(!IsValid());
  assert(fd_ == fd || fd_ == -1);
  fd_ = fd;
  watcher_.Set(fd_, kind_ == Kind::kRead ? EV_READ : EV_WRITE);
  is_valid_ = true;
}

void Direction::StopWatcher() { watcher_.Stop(); }

void Direction::WakeupWaiters() {
  engine::impl::WaitList::Lock lock(*waiters_);
  waiters_->WakeupAll(lock);
}

void Direction::Invalidate() {
  StopWatcher();
  is_valid_ = false;
}

void Direction::IoWatcherCb(struct ev_loop*, ev_io* watcher, int) {
  auto self = static_cast<Direction*>(watcher->data);
  // Watcher::Stop() from ev loop should execute synchronously w/o waiting
  self->StopWatcher();
  self->WakeupWaiters();
}

FdControl::FdControl()
    : read_(Direction::Kind::kRead), write_(Direction::Kind::kWrite) {}

FdControlHolder FdControl::Adopt(int fd) {
  auto& fd_control = Get(fd);
  assert(!fd_control.IsValid());

  SetNonblock(fd);
  fd_control.read_.Reset(fd);
  fd_control.write_.Reset(fd);
  return FdControlHolder(&fd_control);
}

FdControl& FdControl::Get(int fd) {
  static const int nofile_limit = GetNofileLimit();
  static const auto storage = std::make_unique<FdControl[]>(nofile_limit);

  // man-pages clarify that open and socket return the lowest-numbered file
  // descriptor not currently open for the process on success. This holds in
  // practice. Because of this, `fd` should not exceed RLIMIT_NOFILE.
  if (fd >= nofile_limit) {
    throw std::runtime_error(utils::impl::ToString(
        "File descriptor number ", fd, " is too high, max=", nofile_limit - 1));
  }
  return storage[fd];
}

void FdControl::Close() {
  if (!IsValid()) return;
  Invalidate();

  const auto fd = Fd();
  if (::close(fd) == -1) {
    std::error_code ec(errno, std::system_category());
    LOG_ERROR() << "Cannot close fd " << fd << ": " << ec.message();
  }

  read_.WakeupWaiters();
  write_.WakeupWaiters();
}

void FdControl::Invalidate() {
  read_.Invalidate();
  write_.Invalidate();
}

}  // namespace impl
}  // namespace io
}  // namespace engine
