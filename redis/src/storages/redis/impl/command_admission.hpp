#pragma once

#include <atomic>
#include <limits>
#include <optional>
#include <utility>

#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

/// Prevents new producers from entering a short handoff section while allowing
/// shutdown to wait for producers that entered it before `Close()`.
class CommandAdmission final {
public:
    /// A move-only proof that a producer was admitted. Its destructor releases
    /// exactly the permit acquired by `TryAcquire()`.
    ///
    /// `CommandAdmission` must outlive all its permits. Shutdown guarantees this
    /// by calling `Close()` followed by `WaitForNoActivePermits()`.
    class Permit final {
    public:
        Permit(Permit&& other) noexcept
            : admission_(std::exchange(other.admission_, nullptr))
        {}

        Permit(const Permit&) = delete;
        Permit& operator=(const Permit&) = delete;
        Permit& operator=(Permit&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            if (admission_) {
                admission_->Release();
            }
            admission_ = std::exchange(other.admission_, nullptr);
            return *this;
        }

        ~Permit() {
            if (admission_) {
                admission_->Release();
            }
        }

    private:
        friend class CommandAdmission;

        explicit Permit(CommandAdmission& admission) noexcept
            : admission_(&admission)
        {}

        CommandAdmission* admission_;
    };

    /// Atomically acquires a producer permit unless admission has been closed.
    /// If this operation wins the race with `Close()`, the returned permit is
    /// included in the active count observed by `WaitForNoActivePermits()`.
    [[nodiscard]] std::optional<Permit> TryAcquire() noexcept {
        auto state = state_.load(std::memory_order_relaxed);
        while (!GetBit(state, kClosedBit)) {
            if ((state & kActiveCountMask) == kActiveCountMask) {
                UASSERT_MSG(false, "CommandAdmission active permit counter overflow");
                return std::nullopt;
            }
            if (state_.compare_exchange_weak(state, state + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                return Permit{*this};
            }
        }
        return std::nullopt;
    }

    /// Permanently closes admission. Returns `true` only for the first closer.
    bool Close() noexcept { return !GetBit(SetBit(kClosedBit), kClosedBit); }

    bool IsClosed() const noexcept { return GetBit(state_.load(std::memory_order_acquire), kClosedBit); }

    /// After `Close()`, waits until all permits acquired before the close have
    /// been destroyed. This waits only for producer handoff sections, not for
    /// queued commands, Redis replies, or command execution. The calling thread
    /// is parked until the last permit notifies it; the wait does not poll.
    void WaitForNoActivePermits() const noexcept {
        UASSERT(!engine::current_task::IsTaskProcessorThread());
        UASSERT(IsClosed());
        auto state = state_.load(std::memory_order_acquire);
        while (state & kActiveCountMask) {
            state_.wait(state, std::memory_order_acquire);
            state = state_.load(std::memory_order_acquire);
        }
    }

private:
    using State = std::size_t;

    static constexpr State kClosedBit = State{1} << (std::numeric_limits<State>::digits - 1);
    static constexpr State kActiveCountMask = ~kClosedBit;

    static bool GetBit(State state, State bit) noexcept { return state & bit; }

    State SetBit(State bit) noexcept { return state_.fetch_or(bit, std::memory_order_acq_rel); }

    void Release() noexcept {
        const auto old_state = state_.fetch_sub(1, std::memory_order_release);
        UASSERT_MSG(old_state & kActiveCountMask, "Releasing a CommandAdmission permit that is not active");
        if (GetBit(old_state, kClosedBit) && (old_state & kActiveCountMask) == 1) {
            state_.notify_all();
        }
    }

    std::atomic<State> state_{0};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
