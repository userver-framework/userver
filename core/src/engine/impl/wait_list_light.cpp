#include "wait_list_light.hpp"

#include <cstddef>
#include <cstdint>

#include <fmt/format.h>

#include <engine/task/sleep_state.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::impl::AwaiterWithContext> {
    static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(USERVER_NAMESPACE::engine::impl::AwaiterWithContext awaiter, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "({}, {})", fmt::ptr(awaiter.awaiter), awaiter.context);
    }
};

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
namespace {

Awaiter* const kSignaled = reinterpret_cast<Awaiter*>(1);

void DoNotify(AwaiterWithContext awaiter) {
    impl::Notify(boost::intrusive_ptr<Awaiter>{awaiter.awaiter, /*add_ref=*/false}, awaiter.context);
}

}  // namespace

WaitListLight::WaitListLight() noexcept = default;

WaitListLight::~WaitListLight() {
    UASSERT_MSG(IsEmptyRelaxed(), "Someone is awaiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<impl::Awaiter>&& awaiter, std::uintptr_t context) noexcept {
    GetSignalOrAppend(awaiter, context);
    UASSERT_MSG(!awaiter, "Signals cannot be used with plain Append");
}

void WaitListLight::GetSignalOrAppend(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context) noexcept {
    UASSERT(awaiter);

    const AwaiterWithContext new_awaiter{awaiter.get(), context};

    AwaiterWithContext expected{};
    // seq_cst is important for the "Append-Check-Wakeup" sequence.
    const bool success =
        state_.compare_exchange_strong<std::memory_order_seq_cst, std::memory_order_relaxed>(expected, new_awaiter);
    if (!success) {
        UASSERT_MSG(
            expected.awaiter == kSignaled,
            fmt::format(
                "Attempting to wait in a single atomic Awaiter "
                "from multiple coroutines: new={} existing={}",
                new_awaiter,
                expected
            )
        );
        return;
    }

    // Keep a reference logically stored in the WaitListLight to ensure that
    // WakeupOne can complete safely in parallel with the awaiting task being
    // cancelled, Remove-d and stopped.
    awaiter.detach();
}

void WaitListLight::NotifyOne() {
    // seq_cst is important for the "Append-Check-Notify" sequence.
    const auto old_awaiter = state_.exchange<std::memory_order_seq_cst>(AwaiterWithContext{});
    if (old_awaiter.awaiter == nullptr) {
        return;
    }

    UASSERT_MSG(old_awaiter.awaiter != kSignaled, "Use SetSignalAndNotifyOne for dealing with signals instead");

    DoNotify(old_awaiter);
}

void WaitListLight::SetSignalAndNotifyOne() {
    // seq_cst is important for the "Append-Check-Notify" sequence.
    const auto old_awaiter = state_.exchange<std::memory_order_seq_cst>(AwaiterWithContext{kSignaled, {}});
    if (old_awaiter.awaiter == nullptr || old_awaiter.awaiter == kSignaled) {
        return;
    }

    DoNotify(old_awaiter);
}

void WaitListLight::Remove(Awaiter& awaiter, std::uintptr_t context) noexcept {
    const AwaiterWithContext expected{&awaiter, context};

    auto old_awaiter = expected;
    const bool success = state_.compare_exchange_strong<
        std::memory_order_release,
        std::memory_order_relaxed>(old_awaiter, AwaiterWithContext{});

    if (!success) {
        UASSERT_MSG(
            old_awaiter.awaiter == nullptr || old_awaiter.awaiter == kSignaled,
            fmt::format(
                "An unexpected awaiter is occupying the "
                "WaitListLight: expected={} actual={}",
                expected,
                old_awaiter
            )
        );
        return;
    }

    intrusive_ptr_release(&awaiter);
}

bool WaitListLight::GetAndResetSignal() noexcept {
    AwaiterWithContext expected{kSignaled, {}};
    const bool success = state_.compare_exchange_strong<
        std::memory_order_relaxed,
        std::memory_order_relaxed>(expected, AwaiterWithContext{});

    if (!success && expected.awaiter != nullptr) {
        utils::AbortWithStacktrace(
            fmt::format("ResetSignal with an active awaiter is not allowed: awaiter={}", expected)
        );
    }

    return success;
}

bool WaitListLight::IsSignaled() const noexcept {
    const auto torn_awaiter = state_.LoadWithTearing();
    std::atomic_thread_fence(std::memory_order_acquire);
    return torn_awaiter.awaiter == kSignaled;
}

bool WaitListLight::IsEmptyRelaxed() noexcept {
    const auto awaiter = state_.load<std::memory_order_relaxed>();
    return awaiter.awaiter == nullptr || awaiter.awaiter == kSignaled;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
