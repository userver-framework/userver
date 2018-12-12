#include "fd_control.hpp"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cassert>
#include <memory>
#include <stdexcept>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/check_syscall.hpp>

#include <engine/task/task_context.hpp>

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

std::string ToString(Direction::Kind direction_kind) {
  switch (direction_kind) {
    case Direction::Kind::kRead:
      return "read";
    case Direction::Kind::kWrite:
      return "write";
  }
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

bool Direction::Wait(Deadline deadline) {
  assert(IsValid());

  if (engine::current_task::ShouldCancel()) {
    throw IoCancelled("Wait " + ToString(kind_) + "able");
  }

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

  return caller_ctx->GetWakeupSource() ==
         engine::impl::TaskContext::WakeupSource::kWaitList;
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

FdControl::~FdControl() { Close(); }

FdControlHolder FdControl::Adopt(int fd) {
  auto fd_control = std::make_shared<FdControl>();
  SetNonblock(fd);
  fd_control->read_.Reset(fd);
  fd_control->write_.Reset(fd);
  return fd_control;
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
