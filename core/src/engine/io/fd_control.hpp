#pragma once

#include <atomic>
#include <cerrno>

#include <engine/deadline.hpp>
#include <engine/io/error.hpp>
#include <engine/io/fd_control_holder.hpp>
#include <engine/mutex.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

#include <engine/ev/watcher.hpp>
#include <engine/task/task_context.hpp>
#include <engine/wait_list.hpp>

namespace engine {
namespace io {
namespace impl {

/// I/O operation transfer mode
enum class TransferMode {
  kPartial,  ///< operation may complete after transferring any amount of data
  kWhole  ///< operation may complete only after the whole buffer is transferred
};

class FdControl;

class Direction final {
 public:
  enum class Kind { kRead, kWrite };

  class Lock final {
   public:
    explicit Lock(Direction& dir) : impl_(dir.mutex_) {}

   private:
    std::lock_guard<Mutex> impl_;
  };

  Direction(const Direction&) = delete;
  Direction(Direction&&) = delete;
  Direction& operator=(const Direction&) = delete;
  Direction& operator=(Direction&&) = delete;

  explicit operator bool() const { return IsValid(); }
  bool IsValid() const { return is_valid_; }

  int Fd() const { return fd_; }

  [[nodiscard]] bool Wait(Deadline);

  // (IoFunc*)(int, void*, size_t), e.g. read
  template <typename IoFunc, typename... Context>
  size_t PerformIo(Lock& lock, IoFunc&& io_func, void* buf, size_t len,
                   TransferMode mode, Deadline deadline,
                   const Context&... context);

 private:
  friend class FdControl;
  explicit Direction(Kind kind);

  void Reset(int fd);
  void StopWatcher();
  void WakeupWaiters();

  // does not notify
  void Invalidate();

  static void IoWatcherCb(struct ev_loop*, ev_io*, int);

  int fd_;
  const Kind kind_;
  std::atomic<bool> is_valid_;
  Mutex mutex_;
  std::shared_ptr<engine::impl::WaitList> waiters_;
  ev::Watcher<ev_io> watcher_;
};

class FdControl final {
 public:
  // fd will be silently forced to nonblocking mode
  static FdControlHolder Adopt(int fd);

  FdControl();
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~FdControl();

  explicit operator bool() const { return IsValid(); }
  bool IsValid() const { return read_.IsValid(); }

  int Fd() const { return read_.Fd(); }

  Direction& Read() {
    UASSERT(IsValid());
    return read_;
  }
  Direction& Write() {
    UASSERT(IsValid());
    return write_;
  }

  void Close();

  // does not close, must have no waiting in progress
  void Invalidate();

 private:
  Direction read_;
  Direction write_;
};

template <typename IoFunc, typename... Context>
size_t Direction::PerformIo(Lock&, IoFunc&& io_func, void* buf, size_t len,
                            TransferMode mode, Deadline deadline,
                            const Context&... context) {
  char* const begin = static_cast<char*>(buf);
  char* const end = begin + len;

  char* pos = begin;

  while (pos < end) {
    auto chunk_size = io_func(fd_, pos, end - pos);

    if (chunk_size > 0) {
      pos += chunk_size;
    } else if (!chunk_size) {
      break;
    } else if (errno == EINTR) {
      continue;
    } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
      if (pos != begin && mode == TransferMode::kPartial) {
        break;
      }
      if (current_task::ShouldCancel()) {
        throw(IoCancelled() << ... << context);
      }
      [[maybe_unused]] auto is_ready = Wait(deadline);
      if (current_task::GetCurrentTaskContext()->GetWakeupSource() ==
          engine::impl::TaskContext::WakeupSource::kDeadlineTimer) {
        throw IoTimeout(/*bytes_transferred =*/pos - begin);
      }
      if (!IsValid()) {
        throw((IoError() << "Fd closed during ") << ... << context);
      }
    } else {
      const auto err_value = errno;
      IoSystemError ex(err_value);
      ex << "Error while ";
      (ex << ... << context);
      ex << ", fd=" << fd_;
      auto log_level = logging::Level::kError;
      if (err_value == ECONNRESET || err_value == EPIPE) {
        log_level = logging::Level::kWarning;
      }
      LOG(log_level) << ex;
      if (pos != begin) {
        break;
      }
      throw std::move(ex);
    }
  }
  return pos - begin;
}

}  // namespace impl
}  // namespace io
}  // namespace engine
