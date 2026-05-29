#pragma once

#include <cstdint>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <concurrent/impl/fast_atomic.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class Awaiter;
struct AwaiterWithContext;

struct alignas(sizeof(std::uintptr_t) * 2) AwaiterWithContextPtrAndEpoch final {
    AwaiterWithContext* awaiter_with_context{nullptr};
    std::uintptr_t epoch{0};
};

/// Wait list that preserves epoch across operations and only notifies the awaiter if the epoch matches.
class WaitListLightWithEpoch final {
public:
    explicit WaitListLightWithEpoch(std::uint32_t epoch) noexcept;

    WaitListLightWithEpoch(const WaitListLightWithEpoch&) = delete;
    WaitListLightWithEpoch(WaitListLightWithEpoch&&) = delete;
    WaitListLightWithEpoch& operator=(const WaitListLightWithEpoch&) = delete;
    WaitListLightWithEpoch& operator=(WaitListLightWithEpoch&&) = delete;
    ~WaitListLightWithEpoch();

    /// Atomically set the epoch and clear any signal. Should not be used with an awaiter stored.
    /// Returns prior signaled state.
    bool SetEpochThenGetAndResetSignal(std::uint32_t epoch) noexcept;

    /// Relaxed epoch load for the awaiter side.
    std::uint32_t GetEpoch() const noexcept;

    /// Atomically get signal or append with context, using the current epoch.
    /// Only moves from `awaiter` if the awaiter has been appended.
    void GetSignalOrAppend(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context);

    /// Remove awaiter while preserving the epoch.
    void Remove(Awaiter& awaiter, std::uintptr_t context) noexcept;

    /// Atomically get and reset signal while preserving the epoch.
    bool GetAndResetSignal() noexcept;

    /// Atomically set signal and notify if epoch matches.
    void SetSignalAndNotifyOneIfEpochMatches(std::uint32_t epoch) noexcept;

    /// Check if signaled.
    bool IsSignaled() const noexcept;

private:
    concurrent::impl::FastAtomic<AwaiterWithContextPtrAndEpoch> state_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
