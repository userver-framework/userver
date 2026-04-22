#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <engine/task/task_context.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitAnyWaitStrategy final : public WaitStrategy {
public:
    WaitAnyWaitStrategy(utils::span<ContextAccessor*> targets, TaskContext& awaiter)
        : awaiter_(awaiter),
          targets_(targets)
    {}

    EarlyNotify SetupWakeups() override {
        for (auto*& target : targets_) {
            if (!target) {
                continue;
            }

            utils::FastScopeGuard disable_wakeups([&]() noexcept {
                DoDisableWakeups(utils::span{targets_.data(), &target});
            });

            // SetupWakeups might throw.
            boost::intrusive_ptr<Awaiter> awaiter_ptr{&awaiter_};
            target->TryAppendAwaiter(awaiter_ptr, awaiter_.GetAwaiterContext());
            if (awaiter_ptr != nullptr) {  // target is ready.
                return EarlyNotify::kYes;
            }

            disable_wakeups.Release();
        }

        return EarlyNotify::kNo;
    }

    void DisableWakeups() noexcept override { DoDisableWakeups(targets_); }

private:
    void DoDisableWakeups(utils::span<ContextAccessor*> targets) const noexcept {
        for (auto* const target : targets) {
            if (!target) {
                continue;
            }
            target->RemoveAwaiter(awaiter_, awaiter_.GetAwaiterContext());
        }
    }

    TaskContext& awaiter_;
    const utils::span<ContextAccessor*> targets_;
};

inline bool AreUniqueValues(utils::span<ContextAccessor*> targets) {
    std::vector<ContextAccessor*> sorted;
    sorted.reserve(targets.size());
    std::ranges::copy_if(targets, std::back_inserter(sorted), [](const auto& target) { return target != nullptr; });
    std::ranges::sort(sorted);
    return std::ranges::adjacent_find(sorted) == sorted.end();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
