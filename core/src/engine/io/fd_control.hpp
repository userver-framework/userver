#pragma once

#include <sys/uio.h>
#include <atomic>
#include <cerrno>

#include <boost/container/small_vector.hpp>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/fd_control_holder.hpp>
#include <userver/engine/io/fd_poller.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/iovec_advance.hpp>

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
        constexpr explicit SingleUserGuard(Direction&) noexcept {}
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
    ~Direction() = default;

    explicit operator bool() const noexcept { return static_cast<bool>(poller_); }
    bool IsValid() const noexcept { return poller_.IsValid(); }

    int Fd() const noexcept { return poller_.GetFd(); }

    [[nodiscard]] bool Wait(Deadline deadline) { return poller_.Wait(deadline).has_value(); }

    void ResetReady() noexcept { poller_.ResetReady(); }

    // (IoFunc*)(int, void*, size_t), e.g. read
    template <typename IoFunc, typename... Context>
    size_t PerformIo(
        SingleUserGuard& guard,
        IoFunc&& io_func,
        void* buf,
        size_t len,
        TransferMode mode,
        Deadline deadline,
        const Context&... context
    );

    template <typename IoFunc, typename... Context>
    size_t PerformIoV(
        SingleUserGuard& guard,
        IoFunc&& io_func,
        const struct iovec* list,
        std::size_t list_size,
        TransferMode mode,
        Deadline deadline,
        const Context&... context
    );

    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return poller_.GetAwaitableToken();
    }

private:
    friend class FdControl;

    explicit Direction(const ev::ThreadControl& control)
        : poller_(control)
    {}

    void Reset(int fd, Kind kind) { poller_.Reset(fd, kind); }

    // does not notify
    void Invalidate() { poller_.Invalidate(); }

    template <typename IoFunc, typename... Context>
    size_t PerformIoVMutatingTrampoline(
        IoFunc&& io_func,
        utils::IovIter iter,
        TransferMode mode,
        Deadline deadline,
        const Context&... context
    );

    template <typename IoFunc, typename... Context>
    size_t PerformIoVMutating(
        IoFunc&& io_func,
        struct iovec* list,
        std::size_t list_size,
        TransferMode mode,
        Deadline deadline,
        const Context&... context
    );

    template <typename... Context>
    ErrorMode TryHandleError(
        int error_code,
        size_t processed_bytes,
        TransferMode mode,
        Deadline deadline,
        Context&... context
    );

    FdPoller poller_;
};

class FdControl final {
public:
    // fd will be silently forced to nonblocking mode
    static FdControlHolder Adopt(int fd);

    explicit FdControl(const ev::ThreadControl& control);
    ~FdControl();

    explicit operator bool() const { return IsValid(); }
    bool IsValid() const { return read_.IsValid(); }

    int Fd() const noexcept { return read_.Fd(); }

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
size_t Direction::PerformIo(
    SingleUserGuard&,
    IoFunc&& io_func,
    void* buf,
    size_t len,
    TransferMode mode,
    Deadline deadline,
    const Context&... context
) {
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
        } else if (!chunk_size || TryHandleError(errno, pos - begin, mode, deadline, context...) == ErrorMode::kFatal) {
            break;
        }
    }
    return pos - begin;
}

template <typename IoFunc, typename... Context>
size_t Direction::PerformIoV(
    SingleUserGuard&,
    IoFunc&& io_func,
    const struct iovec* list,
    std::size_t list_size,
    TransferMode mode,
    Deadline deadline,
    const Context&... context
) {
    if (list_size == 0) {
        return 0;
    }

    std::size_t processed_bytes = 0;
    do {
        const auto chunk_size = io_func(Fd(), list, (list_size < IOV_MAX ? list_size : IOV_MAX));

        if (chunk_size > 0) {
            processed_bytes += chunk_size;
            if (mode == TransferMode::kOnce) {
                break;
            }

            utils::IovIter iter{list, list_size};
            utils::Advance(iter, chunk_size);
            if (0 == iter.iov_offset) {
                list = iter.iov;
                list_size = iter.iov_size;
            } else [[unlikely]] {
                // we need modify `list` item
                return processed_bytes +
                       PerformIoVMutatingTrampoline(std::forward<IoFunc>(io_func), iter, mode, deadline, context...);
            }
        } else if (!chunk_size ||
                   TryHandleError(errno, processed_bytes, mode, deadline, context...) == ErrorMode::kFatal)
        {
            break;
        }
    } while (list_size != 0);
    return processed_bytes;
}

template <typename IoFunc, typename... Context>
size_t Direction::PerformIoVMutatingTrampoline(
    IoFunc&& io_func,
    utils::IovIter iter,
    TransferMode mode,
    Deadline deadline,
    const Context&... context
) {
    static constexpr std::size_t kReasonableSize = 1024 / sizeof(struct iovec);  // 1KB for the on stack buffer
    boost::container::small_vector<struct iovec, kReasonableSize> buffer_mutable(iter.iov, iter.iov + iter.iov_size);
    utils::Advance(buffer_mutable[0], iter.iov_offset);
    return PerformIoVMutating(
        std::forward<IoFunc>(io_func),
        buffer_mutable.data(),
        buffer_mutable.size(),
        mode,
        deadline,
        context...
    );
}

template <typename IoFunc, typename... Context>
size_t Direction::PerformIoVMutating(
    IoFunc&& io_func,
    struct iovec* list,
    std::size_t list_size,
    TransferMode mode,
    Deadline deadline,
    const Context&... context
) {
    if (list_size == 0) {
        return 0;
    }

    std::size_t processed_bytes = 0;
    do {
        const auto chunk_size = io_func(Fd(), list, (list_size < IOV_MAX ? list_size : IOV_MAX));

        if (chunk_size > 0) {
            processed_bytes += chunk_size;

            utils::IovIter iter{list, list_size};
            utils::Advance(iter, chunk_size);
            list += (iter.iov - list);
            list_size = iter.iov_size;
            if (0 < iter.iov_offset) [[unlikely]] {
                utils::Advance(*list, iter.iov_offset);
            }
        } else if (!chunk_size ||
                   TryHandleError(errno, processed_bytes, mode, deadline, context...) == ErrorMode::kFatal)
        {
            break;
        }
    } while (list_size != 0);

    return processed_bytes;
}

template <typename... Context>
ErrorMode Direction::TryHandleError(
    int error_code,
    size_t processed_bytes,
    TransferMode mode,
    Deadline deadline,
    Context&... context
) {
    if (error_code == EINTR) {
        return ErrorMode::kProcessed;
    } else if (error_code == EWOULDBLOCK
#if EWOULDBLOCK != EAGAIN
               || error_code == EAGAIN
#endif
    )
    {
        if (processed_bytes != 0 && mode != TransferMode::kWhole) {
            return ErrorMode::kFatal;
        }
        if (current_task::ShouldCancel()) {
            throw(IoCancelled(/*bytes_transferred =*/processed_bytes) << ... << context);
        }
        if (!poller_.Wait(deadline)) {
            if (current_task::ShouldCancel()) {
                throw(IoCancelled(/*bytes_transferred =*/processed_bytes) << ... << context);
            } else {
                throw(IoTimeout(/*bytes_transferred =*/processed_bytes) << ... << context);
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
            log_level = logging::Level::kInfo;
        }
        LOG(log_level) << ex;
        if (processed_bytes != 0) {
            return ErrorMode::kFatal;
        }
        throw std::move(ex);
    }
    return ErrorMode::kProcessed;
}

}  // namespace engine::io::impl

USERVER_NAMESPACE_END
