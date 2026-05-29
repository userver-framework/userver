#pragma once

/// @file userver/concurrent/background_task_storage.hpp
/// @brief @copybrief concurrent::BackgroundTaskStorage

#include <cstdint>
#include <utility>

#include <userver/engine/impl/detached_tasks_sync_block.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency userver_containers
///
/// A version of concurrent::BackgroundTaskStorage for advanced use cases (e.g.
/// driver internals) that can take the ownership of any kind of task.
class BackgroundTaskStorageCore final {
public:
    /// Creates an empty BTS.
    BackgroundTaskStorageCore();

    BackgroundTaskStorageCore(BackgroundTaskStorageCore&&) = delete;
    BackgroundTaskStorageCore& operator=(BackgroundTaskStorageCore&&) = delete;
    ~BackgroundTaskStorageCore();

    /// Explicitly cancel and wait for the tasks. New tasks must not be launched
    /// after this call returns. Should be called no more than once.
    void CancelAndWait() noexcept;

    /// Explicitly wait for execution tasks in the store.
    /// Should be called no more than once.
    void CloseAndWaitDebug() noexcept;

    /// @brief Detaches task, allowing it to continue execution out of scope. It
    /// will be cancelled and waited for on BTS destruction.
    /// @note After detach, Task becomes invalid
    ///
    /// Can be called from a coroutine or a non-coroutine thread.
    void Detach(engine::Task&& task);

    /// Approximate number of currently active tasks
    std::int64_t ActiveTasksApprox() const noexcept;

private:
    std::optional<engine::impl::DetachedTasksSyncBlock> sync_block_;
};

/// @ingroup userver_concurrency userver_containers
///
/// A storage that allows one to start detached tasks; cancels and waits for
/// unfinished tasks completion at the destructor. Provides CancelAndWait to
/// explicitly cancel tasks (recommended).
///
/// Usable for detached tasks that capture references to resources with a
/// limited lifetime. You must guarantee that the resources are available while
/// the BackgroundTaskStorage is alive.
///
/// ## Performance considerations
///
/// Tasks remove themselves from the BackgroundTaskStorage on completion, so there is no memory leak.
///
/// @warning The implementation is optimized for spawning tasks. Waiting for remaining tasks may be slow CPU-wise.
/// As a guideline, do not create `BackgroundTaskStorage` instances for each request, use `std::vector<Task>`
/// for storing per-request child tasks instead.
///
/// ## Usage synopsis
///
/// @snippet concurrent/background_task_storage_test.cpp  Sample
///
/// ## Lifetime of task's captures
///
/// All the advice from utils::Async is applicable here.
///
/// BackgroundTaskStorage is always stored as a class field. Tasks that are
/// launched inside it (or moved inside it, for BackgroundTaskStorageCore)
/// can safely access fields declared before it, but not after it:
///
/// @snippet concurrent/background_task_storage_test.cpp  BtsLifetimeCapturesPitfalls
///
/// Generally, it's a good idea to declare `bts_` after most other fields
/// to avoid lifetime bugs. An example of fool-proof code:
///
/// @code
/// private:
///     Foo foo_;
///     Bar bar_;
///
///     // bts_ must be the last field for lifetime reasons.
///     concurrent::BackgroundTaskStorage bts_;
/// };
/// @endcode
///
/// Components and their clients can always be safely captured by reference:
///
/// @see @ref scripts/docs/en/userver/component_system.md
///
/// So for a BackgroundTaskStorage stored in a component, its tasks can only
/// safely use the fields declared before the BTS field, as well as everything
/// from the components, on which the current component depends.
class BackgroundTaskStorage final {
public:
    /// Creates a BTS that launches tasks in the engine::TaskProcessor used at the
    /// BTS creation.
    BackgroundTaskStorage();

    /// Creates a BTS that launches tasks in the specified engine::TaskProcessor.
    explicit BackgroundTaskStorage(engine::TaskProcessor& task_processor);

    BackgroundTaskStorage(const BackgroundTaskStorage&) = delete;
    BackgroundTaskStorage& operator=(const BackgroundTaskStorage&) = delete;

    /// @brief Explicitly cancel and wait for the tasks. New tasks must not be launched
    /// after this call returns. Should be called no more than once.
    ///
    /// More precisely, new tasks must not be launched once this call is completed, but can be launched in the process
    /// of waiting for the remaining tasks. This means that one detached task is allowed to spawn another detached
    /// task into the same BTS, which will immediately be cancelled and for which `CancelAndWait` will wait as well.
    ///
    /// Can only be called from a coroutine.
    void CancelAndWait() noexcept;

    /// @brief Explicitly stop accepting new tasks and wait for execution tasks in the
    /// store. Should be called no more than once.
    ///
    /// Can only be called from a coroutine.
    void CloseAndWaitDebug() noexcept;

    /// @brief Launch a task that will be cancelled and waited for in the BTS
    /// destructor.
    ///
    /// The task is started as non-Critical, it may be cancelled due to
    /// `TaskProcessor` overload. engine::TaskInheritedVariable instances are not
    /// inherited from the caller except baggage::Baggage. See
    /// utils::AsyncBackground for details.
    ///
    /// Can be called from a coroutine or a non-coroutine thread.
    template <typename... Args>
    void AsyncDetach(std::string name, Args&&... args) {
        core_.Detach(utils::AsyncBackground(std::move(name), task_processor_, std::forward<Args>(args)...));
    }

    /// @brief Launch a task that will be cancelled and waited for in the BTS
    /// destructor.
    ///
    /// Execution of function is guaranteed to start regardless
    /// of engine::TaskProcessor load limits.
    /// engine::TaskInheritedVariable instances are not
    /// inherited from the caller except baggage::Baggage. See
    /// utils::CriticalAsyncBackground for details.
    ///
    /// Can be called from a coroutine or a non-coroutine thread.
    template <typename... Args>
    void CriticalAsyncDetach(std::string name, Args&&... args) {
        core_.Detach(utils::CriticalAsyncBackground(std::move(name), task_processor_, std::forward<Args>(args)...));
    }

    /// Approximate number of currently active tasks
    std::int64_t ActiveTasksApprox() const noexcept;

private:
    BackgroundTaskStorageCore core_;
    engine::TaskProcessor& task_processor_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
