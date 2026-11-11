#include <ugrpc/impl/event_engine/executor.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <optional>
#include <utility>

#include <userver/compiler/thread_local.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

compiler::ThreadLocal is_in_callback{[] { return false; }};

}  // namespace

Executor::Executor(engine::TaskProcessor& task_processor)
    : task_processor_(task_processor)
{}

Executor::~Executor() {
    UINVARIANT(
        state_.load(std::memory_order_acquire) == State::kQuiesced,
        "Call Executor::Quiesce from the shutdown owner before destroying the Executor"
    );
}

void Executor::Run(Closure* closure) noexcept {
    UASSERT(closure);
    Run([closure] { closure->Run(); });
}

void Executor::Spawn(AnyInvocable closure, void (*invoke)(AnyInvocable)) noexcept {
    UASSERT(closure);
    UINVARIANT(
        state_.load(std::memory_order_acquire) != State::kQuiesced,
        "Finish external Run and Post calls before Executor::Quiesce; do not post after it returns"
    );
    Detach(engine::CriticalAsyncNoTracing(
        task_processor_,
        [not_started = utils::FastScopeGuard{[]() noexcept {
             utils::AbortWithStacktrace("A committed EventEngine task was destroyed before execution");
         }},
         closure = std::move(closure),
         invoke]() mutable noexcept {
            not_started.Release();
            invoke(std::exchange(closure, {}));
        }
    ));
}

void Executor::Run(AnyInvocable closure) noexcept {
    Spawn(std::move(closure), [](AnyInvocable callback) { RunCallback(std::move(callback)); });
}

void Executor::Post(AnyInvocable closure) noexcept {
    Spawn(std::move(closure), [](AnyInvocable function) { function(); });
}

void Executor::RunCallback(AnyInvocable callback) noexcept {
    UASSERT(callback);
    UASSERT(engine::current_task::IsTaskProcessorThread());
    RunInCallbackScope([&] {
        auto invocation = std::exchange(callback, {});
        invocation();
    });
}

void Executor::RunInCallbackScope(utils::function_ref<void()> function) noexcept {
    std::optional<engine::TaskCancellationBlocker> cancellation_blocker;
    if (engine::current_task::IsTaskProcessorThread()) {
        cancellation_blocker.emplace();
    }
    auto in_callback = is_in_callback.Use();
    const bool previous = std::exchange(*in_callback, true);
    const utils::FastScopeGuard restore([&]() noexcept { *in_callback = previous; });
    function();
}

bool Executor::IsInCallback() noexcept {
    auto in_callback = is_in_callback.Use();
    return *in_callback;
}

void Executor::Quiesce() noexcept {
    if (state_.load(std::memory_order_acquire) == State::kQuiesced) {
        return;
    }
    UINVARIANT(engine::current_task::IsTaskProcessorThread(), "Call Executor::Quiesce from a coroutine");
    UINVARIANT(!IsInCallback(), "Call Executor::Quiesce outside gRPC callbacks and their capture destructors");
    auto expected = State::kRunning;
    UINVARIANT(
        state_.compare_exchange_strong(expected, State::kDraining, std::memory_order_acq_rel),
        "Only one shutdown owner may call Executor::Quiesce at a time"
    );
    tasks_.WaitAndDisposeSlow();
    state_.store(State::kQuiesced, std::memory_order_release);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

#endif
