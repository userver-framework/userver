#pragma once

/// @file userver/engine/async.hpp
/// @brief TaskWithResult creation helpers

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

class TaskFactory final {
 public:
  static size_t GetTaskContextSize();

  template <template <typename> typename TaskType, typename Function,
            typename... Args>
  [[nodiscard]] static auto MakeTaskWithResult(TaskProcessor& task_processor,
                                        Task::Importance importance,
                                        Deadline deadline, Function&& f,
                                        Args&&... args) {
    using WrappedCallT = decltype(*utils::impl::WrapCall(std::forward<Function>(f),
                                                         std::forward<Args>(args)...));

    const auto task_context_size = GetTaskContextSize();
    constexpr auto wrapped_size = sizeof(WrappedCallT);
    constexpr auto wrapped_alignment = alignof(WrappedCallT);
    // check that alignment of WrappedCall is indeed a power of 2,
    // otherwise std::align is UB
    static_assert(wrapped_alignment > 0
                  && (wrapped_alignment & (wrapped_alignment - 1)) == 0);

    const auto total_alloc_size = task_context_size + wrapped_size + wrapped_alignment;
    auto storage = std::make_unique<char[]>(total_alloc_size);

    void* payload_ptr = storage.get() + task_context_size;
    auto payload_space = total_alloc_size - task_context_size;
    if (!std::align(wrapped_alignment, wrapped_size, payload_ptr, payload_space)) {
      // TODO : really shouldn't happen
      std::abort();
    }
    UASSERT(payload_space >= wrapped_size);

    utils::impl::PlacementNewWrappedCall(payload_ptr, std::forward<Function>(f),
                                         std::forward<Args>(args)...);

    using ResultType = decltype(std::declval<WrappedCallT>().Retrieve());
    return TaskType<ResultType>{task_processor, importance, deadline,
                                std::move(storage),
                                static_cast<utils::impl::WrappedCallBase*>(payload_ptr)};
  }
};

template <template <typename> typename TaskType, typename Function,
          typename... Args>
[[nodiscard]] auto MakeTaskWithResult(TaskProcessor& task_processor,
                                      Task::Importance importance,
                                      Deadline deadline, Function&& f,
                                      Args&&... args) {
  return TaskFactory::MakeTaskWithResult<TaskType>(task_processor, importance, deadline,
                                                   std::forward<Function>(f),
                                                       std::forward<Args>(args)...);
}

}  // namespace impl

/// Runs an asynchronous function call using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Function&& f,
                               Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f),
      std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor,
                                     Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f),
      std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using specified task
/// processor
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Deadline deadline,
                               Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kNormal, deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using specified task
/// processor
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor,
                                     Deadline deadline, Function&& f,
                                     Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kNormal, deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Function&& f, Args&&... args) {
  return AsyncNoSpan(current_task::GetTaskProcessor(),
                     std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Function&& f, Args&&... args) {
  return SharedAsyncNoSpan(current_task::GetTaskProcessor(),
                           std::forward<Function>(f),
                           std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using task processor of the
/// caller
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Deadline deadline, Function&& f,
                               Args&&... args) {
  return AsyncNoSpan(current_task::GetTaskProcessor(), deadline,
                     std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using task processor of the
/// caller
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Deadline deadline, Function&& f,
                                     Args&&... args) {
  return SharedAsyncNoSpan(current_task::GetTaskProcessor(), deadline,
                           std::forward<Function>(f),
                           std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that will start regardless of
/// cancellations using specified task processor
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(TaskProcessor& task_processor,
                                       Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kCritical, {},
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that will start regardless of
/// cancellations using specified task processor
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(TaskProcessor& task_processor,
                                             Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kCritical, {},
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that will start regardless of
/// cancellations using task processor of the caller
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(Function&& f, Args&&... args) {
  return CriticalAsyncNoSpan(current_task::GetTaskProcessor(),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that will start regardless of
/// cancellations using task processor of the caller
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(Function&& f, Args&&... args) {
  return SharedCriticalAsyncNoSpan(current_task::GetTaskProcessor(),
                                   std::forward<Function>(f),
                                   std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that will start regardless of
/// cancellations, using task processor of the caller, with deadline
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(Deadline deadline, Function&& f,
                                       Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      current_task::GetTaskProcessor(), Task::Importance::kCritical, deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace engine

USERVER_NAMESPACE_END
