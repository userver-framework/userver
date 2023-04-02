#pragma once

#include <sys/uio.h>
#include <atomic>
#include <cerrno>

#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/fd_control_holder.hpp>
#include <userver/engine/io/fd_poller.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/task/task_context.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io::impl {

/// I/O operation transfer mode
///
/// A note about `TransferMode::kPartial`:
/// OS developers are very smart people and they understand that syscalls
/// are expensive, so they will try their best to return/send as much data as
/// possible in one go.
/// `TransferMode::kPartial` might lead to very noticeable overhead in syscalls
/// if used carelessly, so you are encouraged to stop for a second and think
/// whether you really need this mode.
enum class TransferMode {
  kPartial,  ///< operation may complete after transferring any amount of data
  kWhole,    ///< operation may complete only after the whole buffer is
             ///< transferred
  kOnce,     ///< operation will complete after the first successful transfer
};

/// Return HandleError in PerformIo
enum class ErrorMode {
  kProcessed,  ///< continue execute operation
  kFatal,      ///< break execute operation
};

class FdControl;

class Direction final {
 public:
  using Kind = FdPoller::Kind;
  using State = FdPoller::State;

  class SingleUserGuard final {
   public:
#ifdef NDEBUG
    explicit constexpr SingleUserGuard(Direction&) noexcept {}
#else
    explicit SingleUserGuard(Direction& dir);
    ~SingleUserGuard();

   private:
    Direction& dir_;
#endif
  };

  Direction(const Direction&) = delete;
  Direction(Direction&&) = delete;
  Direction& operator=(const Direction&) = delete;
  Direction& operator=(Direction&&) = delete;
  ~Direction();

  explicit operator bool() const noexcept { return static_cast<bool>(poller_); }
  bool IsValid() const noexcept { return poller_.IsValid(); }

  int Fd() const { return poller_.GetFd(); }

  [[nodiscard]] bool Wait(Deadline);

  // (IoFunc*)(int, void*, size_t), e.g. read
  template <typename IoFunc, typename... Context>
  size_t PerformIo(SingleUserGuard& guard, IoFunc&& io_func, void* buf,
                   size_t len, TransferMode mode, Deadline deadline,
                   const Context&... context);

  template <typename IoFunc, typename... Context>
  size_t PerformIoV(SingleUserGuard& guard, IoFunc&& io_func,
                    struct iovec* list, std::size_t list_size,
                    TransferMode mode, Deadline deadline,
                    const Context&... context);

 private:
  friend class FdControl;
  explicit Direction(Kind kind);

  void Reset(int fd);
  void StopWatcher();
  void WakeupWaiters() { poller_.WakeupWaiters(); }

  // does not notify
  void Invalidate();

  template <typename... Context>
  ErrorMode TryHandleError(int error_code, size_t processed_bytes,
                           TransferMode mode, Deadline deadline,
                           Context&... context);

  FdPoller poller_;
  Kind kind_;
};

class FdControl final {
 public:
  // fd will be silently forced to nonblocking mode
  static FdControlHolder Adopt(int fd);

  FdControl();
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

template <typename... Context>
ErrorMode Direction::TryHandleError(int error_code, size_t processed_bytes,
                                    TransferMode mode, Deadline deadline,
                                    Context&... context) {
  if (error_code == EINTR) {
    return ErrorMode::kProcessed;
  } else if (error_code == EWOULDBLOCK
#if EWOULDBLOCK != EAGAIN
             || error_code == EAGAIN
#endif
  ) {
    if (processed_bytes != 0 && mode != TransferMode::kWhole) {
      return ErrorMode::kFatal;
    }
    if (current_task::ShouldCancel()) {
      throw(IoCancelled(/*bytes_transferred =*/processed_bytes)
            << ... << context);
    }
    if (!poller_.Wait(deadline)) {
      if (current_task::ShouldCancel()) {
        throw(IoCancelled(/*bytes_transferred =*/processed_bytes)
              << ... << context);
      } else {
        throw(IoTimeout(/*bytes_transferred =*/processed_bytes)
              << ... << context);
      }
    }
    if (!IsValid()) {
      throw((IoException() << "Fd closed during ") << ... << context);
    }
  } else {
    IoSystemError ex(error_code, "Direction::PerformIo");
    ex << "Error while ";
    (ex << ... << context);
    ex << ", fd=" << Fd();
    auto log_level = logging::Level::kError;
    if (error_code == ECONNRESET || error_code == EPIPE) {
      log_level = logging::Level::kWarning;
    }
    LOG(log_level) << ex;
    if (processed_bytes != 0) {
      return ErrorMode::kFatal;
    }
    throw std::move(ex);
  }
  return ErrorMode::kProcessed;
}

template <typename IoFunc, typename... Context>
size_t Direction::PerformIoV(SingleUserGuard&, IoFunc&& io_func,
                             struct iovec* list, std::size_t list_size,
                             TransferMode mode, Deadline deadline,
                             const Context&... context) {
  UASSERT(list_size > 0);
  UASSERT(list_size <= IOV_MAX);
  std::size_t processed_bytes = 0;
  do {
    auto chunk_size = io_func(Fd(), list, list_size);

    if (chunk_size > 0) {
      processed_bytes += chunk_size;
      if (mode == TransferMode::kOnce) {
        break;
      }
      std::size_t offset = chunk_size;
      while (list_size > 0) {
        const std::size_t len = list->iov_len;
        if (offset >= len) {
          ++list;
          offset -= len;
          --list_size;
          UASSERT(list_size != 0 || offset == 0);
        } else {
          list->iov_len -= offset;
          list->iov_base = static_cast<char*>(list->iov_base) + offset;
          break;
        }
      }
    } else if (!chunk_size ||
               TryHandleError(errno, processed_bytes, mode, deadline,
                              context...) == ErrorMode::kFatal) {
      break;
    }
  } while (list_size != 0);
  return processed_bytes;
}

template <typename IoFunc, typename... Context>
size_t Direction::PerformIo(SingleUserGuard&, IoFunc&& io_func, void* buf,
                            size_t len, TransferMode mode, Deadline deadline,
                            const Context&... context) {
  char* const begin = static_cast<char*>(buf);
  char* const end = begin + len;

  char* pos = begin;

  while (pos < end) {
    auto chunk_size = io_func(Fd(), pos, end - pos);

    if (chunk_size > 0) {
      pos += chunk_size;
      if (mode == TransferMode::kOnce) {
        break;
      }
    } else if (!chunk_size || TryHandleError(errno, pos - begin, mode, deadline,
                                             context...) == ErrorMode::kFatal) {
      break;
    }
  }
  return pos - begin;
}

}  // namespace engine::io::impl

USERVER_NAMESPACE_END
