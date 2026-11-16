#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/status/status.h>

#include <atomic>
#include <cstdint>

#include <ugrpc/impl/event_engine/closure.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

class LockfreeEvent final {
public:
    LockfreeEvent() = default;
    LockfreeEvent(const LockfreeEvent&) = delete;
    LockfreeEvent& operator=(const LockfreeEvent&) = delete;
    ~LockfreeEvent();

    void InitEvent() noexcept;
    void DestroyEvent() noexcept;

    bool IsShutdown() const noexcept { return (state_.load(std::memory_order_relaxed) & kShutdownBit) != 0; }
    bool IsDestroyed() const noexcept { return state_.load(std::memory_order_relaxed) == kShutdownBit; }

    void NotifyOn(PosixEngineClosure* closure) noexcept;
    bool SetShutdown(y_absl::Status shutdown_error) noexcept;
    void SetReady() noexcept;

private:
    enum State : std::intptr_t { kClosureNotReady = 0, kClosureReady = 2, kShutdownBit = 1 };

    static y_absl::Status* ShutdownStatusOf(std::intptr_t word) noexcept;

    std::atomic<std::intptr_t> state_{kShutdownBit};
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
