#include "generic_wait_list.hpp"

#include <userver/engine/impl/awaiter.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

auto GenericWaitList::CreateWaitList(Task::WaitMode wait_mode) noexcept {
    using ReturnType = std::variant<WaitListLight, WaitListAndSignal>;
    if (wait_mode == Task::WaitMode::kSingleAwaiter) {
        return ReturnType{std::in_place_type<WaitListLight>};
    }
    UASSERT_MSG(wait_mode == Task::WaitMode::kMultipleAwaiters, "Unexpected Task::WaitMode");
    return ReturnType{std::in_place_type<WaitListAndSignal>};
}

GenericWaitList::GenericWaitList(Task::WaitMode wait_mode) noexcept : awaiters_(CreateWaitList(wait_mode)) {}

void GenericWaitList::GetSignalOrAppend(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context) noexcept {
    utils::Visit(
        awaiters_,  //
        [&awaiter, context](WaitListLight& ws) { ws.GetSignalOrAppend(awaiter, context); },
        [&awaiter, context](WaitListAndSignal& ws) {
            if (ws.signal.load()) {
                return;
            }
            WaitList::Lock lock{ws.wl};
            if (ws.signal.load()) {
                return;
            }
            ws.wl.Append(lock, std::move(awaiter), context);
        }
    );
}

// noexcept: awaiters_ are never valueless_by_exception
void GenericWaitList::Remove(Awaiter& awaiter, std::uintptr_t context) noexcept {
    utils::Visit(
        awaiters_,  //
        [&awaiter, context](WaitListLight& ws) { ws.Remove(awaiter, context); },
        [&awaiter, context](WaitListAndSignal& ws) {
            WaitList::Lock lock{ws.wl};
            ws.wl.Remove(lock, awaiter, context);
        }
    );
}

void GenericWaitList::SetSignalAndNotifyAll() {
    utils::Visit(
        awaiters_,  //
        [](WaitListLight& ws) { ws.SetSignalAndNotifyOne(); },
        [](WaitListAndSignal& ws) {
            if (ws.signal.load()) {
                return;
            }
            WaitList::Lock lock{ws.wl};
            if (ws.signal.load()) {
                return;
            }
            // seq_cst is important for the "Append-Check-Notify" sequence.
            ws.signal.store(true, std::memory_order_seq_cst);
            ws.wl.NotifyAll(lock);
        }
    );
}

bool GenericWaitList::IsSignaled() const noexcept {
    return utils::Visit(
        awaiters_,  //
        [](const WaitListLight& ws) { return ws.IsSignaled(); },
        [](const WaitListAndSignal& ws) { return ws.signal.load(); }
    );
}

bool GenericWaitList::IsShared() const noexcept { return std::holds_alternative<WaitListAndSignal>(awaiters_); }

}  // namespace engine::impl

USERVER_NAMESPACE_END
