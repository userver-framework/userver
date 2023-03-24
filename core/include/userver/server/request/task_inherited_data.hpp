#pragma once

/// @file userver/server/request/task_inherited_data.hpp
/// @brief @copybrief server::request::TaskInheritedData

#include <string>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

/// Server request related types and functions
namespace server::request {

/// @brief Per-request data that should be available inside handlers
struct TaskInheritedData final {
  /// The static path of the handler
  const std::string* path{nullptr};

  /// The method of the request
  const std::string& method;

  /// The time when the request started being handled
  std::chrono::steady_clock::time_point start_time{};

  /// The time when there is no use handling the request anymore
  engine::Deadline deadline;
};

/// @see TaskInheritedData for details on the contents.
///
/// ## Stopping deadline propagation
///
/// By default, deadline header is set for client requests created directly
/// from the handler task, as well as from its child tasks. However, this
/// behavior is highly undesirable for requests from background tasks, which
/// should continue past the deadline of the originally handled request.
///
/// To cut off deadline propagation for such a background child task, call
/// @code
/// server::request::kTaskInheritedData.Erase()
/// @endcode
/// within the task.
///
/// @see concurrent::BackgroundTaskStorage::AsyncDetach does it by default.
inline engine::TaskInheritedVariable<TaskInheritedData> kTaskInheritedData;

/// @brief Returns TaskInheritedData::deadline, or an unreachable
/// engine::Deadline if none was set.
inline engine::Deadline GetTaskInheritedDeadline() noexcept {
  const auto* data = kTaskInheritedData.GetOptional();
  return data ? data->deadline : engine::Deadline{};
}

}  // namespace server::request

USERVER_NAMESPACE_END
