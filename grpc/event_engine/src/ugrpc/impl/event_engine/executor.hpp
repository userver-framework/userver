#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>

#include <atomic>
#include <utility>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/function_ref.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Owns callback and coroutine tasks on the configured TaskProcessor until Quiesce().
/// Run applies gRPC's non-switching callback scope; Detach preserves the task's execution policy.
/// The TaskProcessor and resources borrowed by tasks must outlive Quiesce().
class Executor final {
public:
    using Closure = grpc_event_engine::experimental::EventEngine::Closure;
    using AnyInvocable = y_absl::AnyInvocable<void()>;

    explicit Executor(engine::TaskProcessor& task_processor);

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    /// Quiesce must have completed, including on initialization failure paths.
    ~Executor();

    /// Concurrent. Never invokes inline; another worker may start before Run returns.
    /// The caller owns closure, which may delete itself while running.
    void Run(Closure* closure) noexcept;

    /// Concurrent. Destroys closure and its captures on the target TaskProcessor.
    void Run(AnyInvocable closure) noexcept;

    void Post(AnyInvocable closure) noexcept;

    /// Invokes and destroys a callback in the current coroutine under the gRPC callback scope.
    /// For internal I/O continuations already running as tracked tasks on the target TaskProcessor.
    static void RunCallback(AnyInvocable callback) noexcept;

    /// Runs synchronous work under the gRPC callback scope without taking ownership of it.
    /// Construct, invoke and destroy owned callbacks inside this work, so their captures stay in scope.
    /// Also supports foreign threads that synchronously destroy a cancelled timer callback.
    static void RunInCallbackScope(utils::function_ref<void()> function) noexcept;

    /// Transfers a coroutine task to the same drain as Run callbacks.
    /// Finish external submissions before Quiesce; tracked tasks may submit tracked children.
    void Detach(engine::Task&& task) noexcept { tasks_.Detach(std::move(task)); }

    engine::TaskProcessor& GetTaskProcessor() const noexcept { return task_processor_; }

    /// Includes capture destruction; shutdown owners must wait outside every gRPC callback.
    static bool IsInCallback() noexcept;

    /// Stop external producers and finish their Run and Post calls before entering.
    /// Drains tracked tasks and their recursive submissions without cancellation.
    /// Requires one coroutine waiter outside this executor's tracked tasks and all gRPC callbacks.
    /// Repeated calls after completion are allowed; further Run calls are invalid.
    void Quiesce() noexcept;

private:
    enum class State { kRunning, kDraining, kQuiesced };

    void Spawn(AnyInvocable closure, void (*invoke)(AnyInvocable)) noexcept;

    engine::TaskProcessor& task_processor_;
    std::atomic<State> state_{State::kRunning};
    concurrent::BackgroundTaskStorageCore tasks_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

#endif
