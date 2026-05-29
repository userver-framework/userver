#pragma once

#include <cstdint>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <concurrent/impl/fast_atomic.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class Awaiter;

struct alignas(sizeof(std::uintptr_t) * 2) AwaiterWithContext final {
    Awaiter* awaiter{nullptr};
    std::uintptr_t context{0};
};

/// Wait list for a single entry. All functions are thread-safe.
class WaitListLight final {
public:
    /// Create an empty `WaitListLight`
    WaitListLight() noexcept;

    WaitListLight(const WaitListLight&) = delete;
    WaitListLight(WaitListLight&&) = delete;
    WaitListLight& operator=(const WaitListLight&) = delete;
    WaitListLight& operator=(WaitListLight&&) = delete;
    ~WaitListLight();

    /// @brief Append the task to the `WaitListLight`
    /// @note To account for `NotifyOne()` calls between condition check and
    /// `Sleep` + `Append`, you have to recheck the condition after `Append`
    /// returns in `SetupWakeups`.
    /// @note Must not be used together with `SetSignalAndNotifyOne`.
    void Append(boost::intrusive_ptr<impl::Awaiter>&& awaiter, std::uintptr_t context) noexcept;

    /// @brief Get the signal if one was set by SetSignalAndNotifyOne, else Append.
    ///
    /// Atomically:
    ///
    /// 1. If not `IsSignaled`, then
    ///    * move from `awaiter`;
    ///    * store `awaiter` and `context` to notify when `IsSignaled() == true` is reached.
    /// 2. If `IsSignaled`, then
    ///    * do not move from `awaiter`;
    ///    * do not notify `awaiter`.
    ///
    /// @returns `true` if already signaled
    /// @see Append
    void GetSignalOrAppend(boost::intrusive_ptr<impl::Awaiter>& awaiter, std::uintptr_t context) noexcept;

    /// @brief Remove the task from the `WaitListLight` without notification.
    void Remove(impl::Awaiter& awaiter, std::uintptr_t context) noexcept;

    /// @brief Notifies the waiting task; the next awaiter may not `Append` until
    /// `Remove` is called.
    void NotifyOne();

    /// @brief Sets signal, which will notify future awaiters. Notifies the
    /// existing awaiter, if any. The next awaiter may not `Append` until
    /// `Remove` is called.
    /// @see GetSignalOrAppend
    void SetSignalAndNotifyOne();

    /// @brief Resets the notification, if any.
    /// @warning Reset with an active awaiter is not allowed! A good rule of thumb
    /// is to only call this from the waiting task.
    bool GetAndResetSignal() noexcept;

    /// @returns Whether the signal was set and not yet reset
    bool IsSignaled() const noexcept;

private:
    bool IsEmptyRelaxed() noexcept;

    concurrent::impl::FastAtomic<AwaiterWithContext> state_{AwaiterWithContext{}};
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
