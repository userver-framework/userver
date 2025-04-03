#include "generic_wait_list.hpp"

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

auto GenericWaitList::CreateWaitList(Task::WaitMode wait_mode) noexcept {
    using ReturnType = std::variant<WaitListLight, WaitListAndSignal>;
    if (wait_mode == Task::WaitMode::kSingleWaiter) {
        return ReturnType{std::in_place_type<WaitListLight>};
    }
    UASSERT_MSG(wait_mode == Task::WaitMode::kMultipleWaiters, "Unexpected Task::WaitMode");
    return ReturnType{std::in_place_type<WaitListAndSignal>};
}

GenericWaitList::GenericWaitList(Task::WaitMode wait_mode) noexcept : waiters_(CreateWaitList(wait_mode)) {}

bool GenericWaitList::GetSignalOrAppend(boost::intrusive_ptr<TaskContext>&& context) noexcept {
    return utils::Visit(
        waiters_,  //
        [&context](WaitListLight& ws) { return ws.GetSignalOrAppend(std::move(context)); },
        [&context](WaitListAndSignal& ws) {
            if (ws.signal.load()) return true;
            WaitList::Lock lock{ws.wl};
            if (ws.signal.load()) return true;
            ws.wl.Append(lock, std::move(context));
            return false;
        }
    );
}

// noexcept: waiters_ are never valueless_by_exception
void GenericWaitList::Remove(TaskContext& context) noexcept {
    utils::Visit(
        waiters_,  //
        [&context](WaitListLight& ws) { ws.Remove(context); },
        [&context](WaitListAndSignal& ws) {
            WaitList::Lock lock{ws.wl};
            ws.wl.Remove(lock, context);
        }
    );
}

void GenericWaitList::SetSignalAndWakeupAll() {
    utils::Visit(
        waiters_,  //
        [](WaitListLight& ws) { ws.SetSignalAndWakeupOne(); },
        [](WaitListAndSignal& ws) {
            if (ws.signal.load()) return;
            WaitList::Lock lock{ws.wl};
            if (ws.signal.load()) return;
            // seq_cst is important for the "Append-Check-Wakeup" sequence.
            ws.signal.store(true, std::memory_order_seq_cst);
            ws.wl.WakeupAll(lock);
        }
    );
}

bool GenericWaitList::IsSignaled() const noexcept {
    return utils::Visit(
        waiters_,  //
        [](const WaitListLight& ws) { return ws.IsSignaled(); },
        [](const WaitListAndSignal& ws) { return ws.signal.load(); }
    );
}

bool GenericWaitList::IsShared() const noexcept { return std::holds_alternative<WaitListAndSignal>(waiters_); }

}  // namespace engine::impl

USERVER_NAMESPACE_END
