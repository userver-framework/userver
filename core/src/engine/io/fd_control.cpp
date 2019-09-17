#include "fd_control.hpp"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <stdexcept>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/check_syscall.hpp>

#include <engine/task/task_context.hpp>
#include <utils/assert.hpp>
#include <utils/userver_experiment.hpp>

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

class DirectionWaitStrategy final : public engine::impl::WaitStrategy {
 public:
  DirectionWaitStrategy(Deadline deadline,
                        std::shared_ptr<engine::impl::WaitList> waiters,
                        ev::Watcher<ev_io>& watcher,
                        engine::impl::TaskContext* current)
      : WaitStrategy(deadline),
        waiters_(std::move(waiters)),
        lock_(*waiters_),
        watcher_(watcher),
        current_(current) {}

  void AfterAsleep() override {
    waiters_->Append(lock_, current_);
    lock_.Release();
    watcher_.StartAsync();
  }

  void BeforeAwake() override {}

  std::shared_ptr<engine::impl::WaitListBase> GetWaitList() override {
    return waiters_;
  }

 private:
  const std::shared_ptr<engine::impl::WaitList> waiters_;
  engine::impl::WaitList::Lock lock_;
  ev::Watcher<ev_io>& watcher_;
  engine::impl::TaskContext* const current_;
};

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
  UASSERT(IsValid());

  auto current = current_task::GetCurrentTaskContext();

  if (current->ShouldCancel()) {
    throw IoCancelled() << "Wait " << ToString(kind_) << "able";
  }

  impl::DirectionWaitStrategy wait_manager(deadline, waiters_, watcher_,
                                           current);
  current->Sleep(&wait_manager);

  return current->GetWakeupSource() ==
         engine::impl::TaskContext::WakeupSource::kWaitList;
}

void Direction::Reset(int fd) {
  UASSERT(!IsValid());
  UASSERT(fd_ == fd || fd_ == -1);
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

// NOLINTNEXTLINE(bugprone-exception-escape)
void Direction::IoWatcherCb(struct ev_loop*, ev_io* watcher, int) try {
  auto self = static_cast<Direction*>(watcher->data);
  // Watcher::Stop() from ev loop should execute synchronously w/o waiting
  self->StopWatcher();
  self->WakeupWaiters();
} catch (...) {
  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kTaxicommon1479)) {
    std::cerr << "Uncaught exception in " << __PRETTY_FUNCTION__;
    abort();
  }
  throw;
}

FdControl::FdControl()
    : read_(Direction::Kind::kRead), write_(Direction::Kind::kWrite) {}

// NOLINTNEXTLINE(bugprone-exception-escape)
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
