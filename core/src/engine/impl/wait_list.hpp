#pragma once

#include <atomic>
#include <mutex>

#include <boost/intrusive/list.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/impl/awaiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

/// Wait list for multiple entries with explicit control over critical section.
class WaitList final {
public:
    class Lock final {
    public:
        explicit Lock(WaitList& list) noexcept : impl_(list.mutex_) {}

        explicit operator bool() noexcept { return !!impl_; }

        void lock() { impl_.lock(); }
        void unlock() { impl_.unlock(); }

    private:
        std::unique_lock<std::mutex> impl_;
    };

    // This guard is used to optimize the hot path of unlocking:
    //
    // Use `AwaitersScopeCounter` before acquiring the `WaitList::Lock` and do
    // not destroy it as long as the coroutine  may go to sleep.
    //
    // Now in the `unlock` part call `GetCountOfSleepies()` before
    // `WaitList::Lock + NotifyOne/NotifyAll`.
    class AwaitersScopeCounter final {
    public:
        explicit AwaitersScopeCounter(WaitList& list) noexcept : impl_(list) {
            ++impl_.sleepies_;
        }
        ~AwaitersScopeCounter() { --impl_.sleepies_; }

    private:
        WaitList& impl_;
    };

    /// Create an empty `WaitList`
    WaitList() noexcept;

    WaitList(const WaitList&) = delete;
    WaitList(WaitList&&) = delete;
    WaitList& operator=(const WaitList&) = delete;
    WaitList& operator=(WaitList&&) = delete;
    ~WaitList();

    bool IsEmpty(Lock&) const noexcept;

    /// @brief Append the task to the `WaitList`
    void Append(Lock& lock, boost::intrusive_ptr<impl::Awaiter> awaiter, std::uintptr_t context) noexcept;

    /// @brief Remove the task from the `WaitList` without notifying
    void Remove(Lock& lock, impl::Awaiter& awaiter, std::uintptr_t context) noexcept;

    void NotifyOne(Lock&);
    void NotifyAll(Lock&);

    /// @brief Get the maximum amount of coroutines that may be sleeping
    /// @returns 0 if there are definitely no awaiters currently, non-0 otherwise
    std::size_t GetCountOfSleepies() const noexcept { return sleepies_.load(); }

private:
    using MemberHookConfig =
        boost::intrusive::member_hook<impl::Awaiter, impl::Awaiter::WaitListData, &impl::Awaiter::wait_list_data_>;

    struct List
        : public boost::intrusive::make_list<
              impl::Awaiter,
              boost::intrusive::constant_time_size<false>,
              MemberHookConfig>::type {};

    std::atomic<std::size_t> sleepies_{0};
    std::mutex mutex_;
    List awaiters_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
