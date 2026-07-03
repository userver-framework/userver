#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <engine/task/task_context.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitAnyAwaitable final : public WeakAwaitable {
public:
    explicit WaitAnyAwaitable(utils::span<AwaitableToken> targets)
        : targets_(targets)
    {}

    bool IsReady() const noexcept override {
        return std::ranges::all_of(targets_, [](const AwaitableToken& target) {
            return target.IsEmpty() || target.GetAwaitable(utils::impl::InternalTag{}).IsReady();
        });
    }

    void TryAppendAwaiter(AwaiterPtr& awaiter, std::uintptr_t context) override {
        for (auto& target : targets_) {
            if (target.IsEmpty()) {
                continue;
            }
            auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});

            utils::FastScopeGuard disable_wakeups([&]() noexcept {
                DoDisableWakeups(utils::span{targets_.data(), &target}, *awaiter, context);
            });

            // TryAppendAwaiter might throw.
            auto awaiter_copy = awaiter->CastToTaskContext().AsAwaiterPtr();
            awaitable.TryAppendAwaiter(awaiter_copy, context);
            if (awaiter_copy != nullptr) {  // target is ready.
                return;
            }

            disable_wakeups.Release();
        }

        // Mark that the compound awaitable is not ready. A possible optimization (not performed currently)
        // is to pass `awaiter` to the last target directly instead of copying it.
        awaiter = nullptr;
    }

    AwaiterPtr RemoveAwaiter(Awaiter& awaiter, std::uintptr_t context) noexcept override {
        return DoDisableWakeups(targets_, awaiter, context);
    }

private:
    AwaiterPtr DoDisableWakeups(utils::span<AwaitableToken> targets, Awaiter& awaiter, std::uintptr_t context) const
        noexcept {
        AwaiterPtr compound_removal_result;
        bool removed_before_notification = true;

        for (const auto& target : targets) {
            if (target.IsEmpty()) {
                continue;
            }
            auto& awaitable = target.GetAwaitable(utils::impl::InternalTag{});

            auto removal_result = awaitable.RemoveAwaiter(awaiter, context);
            removed_before_notification &= (removal_result != nullptr);
            compound_removal_result = std::move(removal_result);
        }

        if (!removed_before_notification) {
            compound_removal_result = nullptr;
        }
        return compound_removal_result;
    }

    const utils::span<AwaitableToken> targets_;
};

inline bool AreUniqueValues(utils::span<AwaitableToken> targets) {
    std::vector<AwaitableToken> sorted;
    sorted.reserve(targets.size());
    std::ranges::copy_if(targets, std::back_inserter(sorted), [](const auto& target) { return !target.IsEmpty(); });
    std::ranges::sort(sorted, {}, [](const auto& target) { return &target.GetAwaitable(utils::impl::InternalTag{}); });
    return std::ranges::adjacent_find(sorted) == sorted.end();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
