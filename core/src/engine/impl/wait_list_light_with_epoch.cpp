#include "wait_list_light_with_epoch.hpp"

#include <memory>

#include <fmt/format.h>

#include <engine/impl/wait_list_light.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

AwaiterWithContext* const kSignaled = reinterpret_cast<AwaiterWithContext*>(1);

}  // namespace

WaitListLightWithEpoch::WaitListLightWithEpoch(std::uint32_t epoch) noexcept : state_({nullptr, epoch}) {}

WaitListLightWithEpoch::~WaitListLightWithEpoch() {
    const auto state = state_.load<std::memory_order_relaxed>();
    UASSERT_MSG(
        state.awaiter_with_context == nullptr || state.awaiter_with_context == kSignaled,
        "Someone is awaiting on WaitListLightWithEpoch while it's being destroyed"
    );
}

bool WaitListLightWithEpoch::SetEpochThenGetAndResetSignal(std::uint32_t epoch) noexcept {
    const AwaiterWithContextPtrAndEpoch new_value{nullptr, epoch};
    const auto old_state = state_.exchange<std::memory_order_seq_cst>(new_value);
    UASSERT_MSG(
        old_state.awaiter_with_context == nullptr || old_state.awaiter_with_context == kSignaled,
        "Someone is awaiting on WaitListLightWithEpoch during SetEpoch"
    );
    return old_state.awaiter_with_context == kSignaled;
}

std::uint32_t WaitListLightWithEpoch::GetEpoch() const noexcept { return state_.LoadWithTearing().epoch; }

void WaitListLightWithEpoch::GetSignalOrAppend(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context) {
    UASSERT(awaiter);

    const auto epoch = state_.LoadWithTearing().epoch;

    auto new_awaiter_with_context = std::make_unique<AwaiterWithContext>(AwaiterWithContext{awaiter.get(), context});

    const AwaiterWithContextPtrAndEpoch new_state{new_awaiter_with_context.get(), epoch};

    // Expect either {nullptr, epoch} or {kSignaled, epoch}.
    AwaiterWithContextPtrAndEpoch expected{nullptr, epoch};

    // seq_cst is important for the "Append-Check-Wakeup" sequence.
    bool success =
        state_.compare_exchange_strong<std::memory_order_seq_cst, std::memory_order_relaxed>(expected, new_state);

    if (!success) {
        // CAS failed - new_awaiter_with_context will clean up.

        UASSERT_MSG(
            expected.awaiter_with_context == kSignaled && expected.epoch == epoch,
            "An unexpected awaiter is occupying the WaitListLightWithEpoch or epoch does not match"
        );

        // Already signaled with matching epoch: awaiter is not moved from.
        return;
    }

    // CAS succeeded - release ownership to the atomic.
    [[maybe_unused]] auto* const released_ptr = new_awaiter_with_context.release();

    // Keep a reference logically stored in the WaitListLightWithEpoch.
    awaiter.detach();
}

void WaitListLightWithEpoch::Remove(Awaiter& awaiter, std::uintptr_t context) noexcept {
    const auto current = state_.LoadWithTearing();
    AwaiterWithContextPtrAndEpoch expected = current;

    UASSERT(expected.awaiter_with_context != nullptr);
    if (expected.awaiter_with_context == kSignaled) {
        // Already removed/signaled.
        return;
    }

    // Preserve the epoch when removing the awaiter.
    const AwaiterWithContextPtrAndEpoch new_value{nullptr, current.epoch};

    const bool success =
        state_.compare_exchange_strong<std::memory_order_acq_rel, std::memory_order_relaxed>(expected, new_value);

    if (!success) {
        UASSERT_MSG(
            expected.awaiter_with_context == nullptr || expected.awaiter_with_context == kSignaled,
            "An unexpected awaiter is occupying the WaitListLightWithEpoch or epoch changed mid-await"
        );
        return;
    }

    UASSERT_MSG(
        expected.awaiter_with_context->awaiter == &awaiter && expected.awaiter_with_context->context == context,
        "An unexpected awaiter is occupying the WaitListLightWithEpoch"
    );

    if (current.awaiter_with_context != nullptr && current.awaiter_with_context != kSignaled) {
        std::default_delete<AwaiterWithContext>{}(current.awaiter_with_context);
    }
    intrusive_ptr_release(&awaiter);
}

bool WaitListLightWithEpoch::GetAndResetSignal() noexcept {
    const auto current = state_.load<std::memory_order_acquire>();

    // Reset signal from {kSignaled, epoch} to {nullptr, epoch}.
    AwaiterWithContextPtrAndEpoch expected{kSignaled, current.epoch};
    const AwaiterWithContextPtrAndEpoch new_value{nullptr, current.epoch};

    const bool success =
        state_.compare_exchange_strong<std::memory_order_relaxed, std::memory_order_relaxed>(expected, new_value);

    if (!success) {
        // Only two states are allowed: {nullptr, epoch} or {kSignaled, epoch}.
        UASSERT_MSG(
            expected.epoch == current.epoch && expected.awaiter_with_context == nullptr,
            "An unexpected awaiter is occupying the WaitListLightWithEpoch or epoch does not match"
        );
    }

    return success;
}

void WaitListLightWithEpoch::SetSignalAndNotifyOneIfEpochMatches(std::uint32_t epoch) noexcept {
    auto current = state_.LoadWithTearing();

    for (;;) {
        if (current.epoch != epoch) {
            return;
        }

        const AwaiterWithContextPtrAndEpoch new_value{kSignaled, epoch};

        const bool success =
            state_.compare_exchange_strong<std::memory_order_seq_cst, std::memory_order_relaxed>(current, new_value);

        if (success) {
            if (current.awaiter_with_context != nullptr && current.awaiter_with_context != kSignaled) {
                boost::intrusive_ptr<Awaiter> awaiter{current.awaiter_with_context->awaiter, /*add_ref=*/false};
                const auto context = current.awaiter_with_context->context;
                std::default_delete<AwaiterWithContext>{}(current.awaiter_with_context);
                impl::Notify(std::move(awaiter), context);
            }
            return;
        }
        // CAS failed, current is updated with actual value, retry.
    }
}

bool WaitListLightWithEpoch::IsSignaled() const noexcept {
    const auto torn_state = state_.LoadWithTearing();
    std::atomic_thread_fence(std::memory_order_acquire);
    return torn_state.awaiter_with_context == kSignaled;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
