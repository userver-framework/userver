#include "io_watcher.hpp"

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

IoWatcher::IoWatcher(ThreadControl& thread_control)
    : watcher_read_(thread_control, this),
      watcher_write_(thread_control, this) {}

IoWatcher::~IoWatcher() {
  Cancel();
  CloseFd();
}

void IoWatcher::SetFd(int fd) {
  Cancel();
  CloseFd();

  fd_ = fd;
}

bool IoWatcher::HasFd() const { return fd_ != -1; }
int IoWatcher::GetFd() const { return fd_; }

int IoWatcher::Release() { return std::exchange(fd_, -1); }

void IoWatcher::ReadAsync(Callback cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_read_);
  }
  if (cb)
    throw std::logic_error(
        "Called ReadAsync() while another read wait is already pending");

  watcher_read_.Init(&IoWatcher::OnEventRead, fd_, EV_READ);
  watcher_read_.Start();
}

void IoWatcher::WriteAsync(Callback cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_write_);
  }
  if (cb)
    throw std::logic_error(
        "Called WriteAsync() while another write wait is already pending");

  watcher_write_.Init(&IoWatcher::OnEventWrite, fd_, EV_WRITE);
  watcher_write_.Start();
}

void IoWatcher::OnEventRead(struct ev_loop*, ev_io* io, int events) noexcept {
  auto* self = static_cast<IoWatcher*>(io->data);
  self->watcher_read_.Stop();

  if (events & EV_READ) {
    try {
      self->CallReadCb(std::error_code());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Uncaught exception in IoWatcher read callback: " << ex;
    }
  }
}

void IoWatcher::CallReadCb(std::error_code ec) {
  Callback cb;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_read_);
  }
  if (cb) {
    cb(ec);
  }
}

void IoWatcher::OnEventWrite(struct ev_loop*, ev_io* io, int events) noexcept {
  auto* self = static_cast<IoWatcher*>(io->data);
  self->watcher_write_.Stop();

  if (events & EV_WRITE) {
    try {
      self->CallWriteCb(std::error_code());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Uncaught exception in IoWatcher write callback: " << ex;
    }
  }
}

void IoWatcher::CallWriteCb(std::error_code ec) {
  Callback cb;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_write_);
  }
  if (cb) {
    cb(ec);
  }
}

void IoWatcher::Cancel() {
  LOG_TRACE() << "IoWatcher::Cancel (1)";
  CancelSingle(watcher_write_, cb_write_);
  LOG_TRACE() << "IoWatcher::Cancel (2)";
  CancelSingle(watcher_read_, cb_read_);
  LOG_TRACE() << "IoWatcher::Cancel (3)";
}

void IoWatcher::CancelSingle(Watcher<ev_io>& watcher, Callback& cb) {
  Callback cb_local;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb_local, cb);
  }
  if (cb_local) {
    watcher.Stop();
    cb_local(std::make_error_code(std::errc::operation_canceled));
  }
}

void IoWatcher::CloseFd() {
  if (fd_ != -1) {
    int rc = close(fd_);
    if (rc)
      LOG_ERROR() << "close(2) failed: "
                  << std::error_code(errno, std::generic_category());
    fd_ = -1;
  }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
