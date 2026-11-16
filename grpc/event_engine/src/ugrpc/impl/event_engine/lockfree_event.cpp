#include <ugrpc/impl/event_engine/lockfree_event.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/status/status.h>

#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

static_assert(alignof(PosixEngineClosure) >= 4, "Bits 0 and 1 of a closure pointer must be free for the state tags");
static_assert(alignof(y_absl::Status) >= 2, "Bit 0 of a status pointer must be free for kShutdownBit");

LockfreeEvent::~LockfreeEvent() {
    UASSERT_MSG(
        state_.load(std::memory_order_relaxed) == kShutdownBit,
        "Call DestroyEvent before destroying a LockfreeEvent"
    );
}

y_absl::Status* LockfreeEvent::ShutdownStatusOf(std::intptr_t word) noexcept {
    return reinterpret_cast<y_absl::Status*>(word & ~static_cast<std::intptr_t>(kShutdownBit));
}

void LockfreeEvent::InitEvent() noexcept { state_.store(kClosureNotReady, std::memory_order_relaxed); }

void LockfreeEvent::DestroyEvent() noexcept {
    std::intptr_t curr = state_.load(std::memory_order_relaxed);
    do {
        if ((curr & kShutdownBit) != 0) {
            delete ShutdownStatusOf(curr);
        } else {
            UASSERT(curr == kClosureNotReady || curr == kClosureReady);
        }
    } while (!state_.compare_exchange_strong(curr, kShutdownBit, std::memory_order_acq_rel, std::memory_order_acquire));
}

void LockfreeEvent::NotifyOn(PosixEngineClosure* closure) noexcept {
    UASSERT(closure);
    std::intptr_t curr = state_.load(std::memory_order_acquire);
    while (true) {
        switch (curr) {
            case kClosureNotReady:
                if (state_.compare_exchange_strong(
                        curr,
                        reinterpret_cast<std::intptr_t>(closure),
                        std::memory_order_acq_rel,
                        std::memory_order_acquire
                    ))
                {
                    return;
                }
                break;
            case kClosureReady:
                if (state_.compare_exchange_strong(
                        curr,
                        kClosureNotReady,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire
                    ))
                {
                    closure->SetStatus(y_absl::OkStatus());
                    closure->Run();
                    return;
                }
                break;
            default:
                if ((curr & kShutdownBit) != 0) {
                    auto* const shutdown_error = ShutdownStatusOf(curr);
                    UINVARIANT(shutdown_error, "LockfreeEvent::NotifyOn after DestroyEvent");
                    closure->SetStatus(*shutdown_error);
                    closure->Run();
                    return;
                }
                utils::AbortWithStacktrace(
                    "LockfreeEvent::NotifyOn: notify_on called with a previous callback still pending"
                );
        }
    }
}

bool LockfreeEvent::SetShutdown(y_absl::Status shutdown_error) noexcept {
    auto* const status_ptr = new y_absl::Status(shutdown_error);
    const auto new_state = reinterpret_cast<std::intptr_t>(status_ptr) | static_cast<std::intptr_t>(kShutdownBit);
    std::intptr_t curr = state_.load(std::memory_order_acquire);
    while (true) {
        switch (curr) {
            case kClosureReady:
            case kClosureNotReady:
                if (state_
                        .compare_exchange_strong(curr, new_state, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return true;
                }
                break;
            default:
                if ((curr & kShutdownBit) != 0) {
                    delete status_ptr;
                    return false;
                }
                if (state_
                        .compare_exchange_strong(curr, new_state, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    auto* const closure = reinterpret_cast<PosixEngineClosure*>(curr);
                    closure->SetStatus(std::move(shutdown_error));
                    closure->Run();
                    return true;
                }
                break;
        }
    }
}

void LockfreeEvent::SetReady() noexcept {
    std::intptr_t curr = state_.load(std::memory_order_acquire);
    while (true) {
        switch (curr) {
            case kClosureReady:
                return;
            case kClosureNotReady:
                if (state_.compare_exchange_strong(
                        curr,
                        kClosureReady,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire
                    ))
                {
                    return;
                }
                break;
            default:
                if ((curr & kShutdownBit) != 0) {
                    return;
                }
                if (state_.compare_exchange_strong(
                        curr,
                        kClosureNotReady,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire
                    ))
                {
                    auto* const closure = reinterpret_cast<PosixEngineClosure*>(curr);
                    closure->SetStatus(y_absl::OkStatus());
                    closure->Run();
                    return;
                }
                return;
        }
    }
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
