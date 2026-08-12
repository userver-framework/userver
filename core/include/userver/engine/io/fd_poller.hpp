#pragma once

/// @file userver/engine/io/fd_poller.hpp
/// @brief Low-level file descriptor r/w poller

#include <optional>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class AwaitableBase;
}

namespace engine::ev {
class ThreadControl;
}

namespace engine::io {

namespace impl {
class Direction;
}

/// @brief Low-level poller that can wait for read, write, and read/write
/// operations on a file descriptor. It does not provide read/write operations.
/// @note FdPoller is not thread safe and its methods must not be called from
/// multiple threads simultaneously.
class FdPoller final {
public:
    /// @brief Operation kind to wait for
    enum class Kind {
        kRead = 1,       /// < wait for read availability
        kWrite = 2,      /// < wait for write availability
        kReadWrite = 3,  /// < wait for either read or write availability
    };

    /// Constructor for FdPoller. `control` parameter could be obtained via
    /// engine::current_task::GetEventThread().
    ///
    /// It is recommended to place read and write FdPoller's of the same FD to
    /// the same `control` for better ev threads balancing.
    explicit FdPoller(const ev::ThreadControl& control);

    FdPoller(const FdPoller&) = delete;
    FdPoller(FdPoller&&) = delete;
    FdPoller& operator=(const FdPoller&) = delete;
    FdPoller& operator=(FdPoller&&) = delete;
    ~FdPoller();

    /// The same as `IsValid()`.
    explicit operator bool() const noexcept;
    /// Whether the file descriptor is valid.
    bool IsValid() const noexcept;

    /// If IsValid(), get file descriptor.
    int GetFd() const noexcept;

    /// When you're done with fd, call Invalidate(). It unregisters the fd, after
    /// that you have to call close(2) by yourself. After Invalidate() you may not
    /// call Wait().
    void Invalidate();

    /// Setup fd and kind to wait for. After Reset() you may call Wait().
    /// FdPoller does not take the ownership of `fd`, you still have to close `fd`
    /// when you're done.
    void Reset(int fd, Kind kind);

    /// Wait for an event kind that was passed in the latest Reset() call. If the
    /// operation (read/write) can already be handled, Wait() returns
    /// immediately. You have to call Reset() at least once before call to
    /// Wait().
    [[nodiscard]] std::optional<Kind> Wait(Deadline);

    /// Reset "ready" flag for WaitAny. Once notified, wait operations will return
    /// immediately on further calls unless readiness is reset.
    void ResetReady() noexcept;

    /// Get event kind that was triggered on this poller.
    /// Resets "ready" flag.
    std::optional<FdPoller::Kind> GetReady() noexcept;

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND;

private:
    friend class impl::Direction;

    enum class State : int {
        kInvalid,
        kReadyToUse,
        kInUse,  /// < used only in debug to detect invalid concurrent usage
    };

    void SwitchStateToInUse();
    void SwitchStateToReadyToUse();

    class Impl;
    utils::FastPimpl<Impl, 144, 16> pimpl_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
