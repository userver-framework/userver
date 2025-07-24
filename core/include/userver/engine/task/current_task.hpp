#pragma once

/// @file userver/engine/task/current_task.hpp
/// @brief Utility functions that query and operate on the current task

#include <cstddef>
#include <cstdint>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace engine {

/// @brief Namespace with functions to work with current task from within it
namespace current_task {

/// Returns true only when running in userver coroutine environment,
/// i.e. in an engine::TaskProcessor thread.
bool IsTaskProcessorThread() noexcept;

/// Returns reference to the task processor executing the caller
TaskProcessor& GetTaskProcessor();

/// Returns reference to the blocking task processor
TaskProcessor& GetBlockingTaskProcessor();

/// Returns task coroutine stack size
std::size_t GetStackSize();

/// @cond
// Returns ev thread handle, internal use only
ev::ThreadControl& GetEventThread();
/// @endcond

namespace impl {
// For internal use only.
void* GetRawCurrentTaskContext() noexcept;
}  // namespace impl

}  // namespace current_task

namespace impl {

// For internal use only.
std::uint64_t GetCreatedTaskCount(TaskProcessor&);

}  // namespace impl

}  // namespace engine

USERVER_NAMESPACE_END
