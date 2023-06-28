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
  std::string_view path;

  /// The method of the request
  std::string_view method;

  /// The time when the request started being handled
  std::chrono::steady_clock::time_point start_time{};

  /// The time when there is no use handling the request anymore
  engine::Deadline deadline;
};

/// @see TaskInheritedData for details on the contents.
inline engine::TaskInheritedVariable<TaskInheritedData> kTaskInheritedData;

/// @brief Returns TaskInheritedData::deadline, or an unreachable
/// engine::Deadline if none was set.
engine::Deadline GetTaskInheritedDeadline() noexcept;

/// @brief Stops deadline propagation within its scope
///
/// By default, handler deadline is honored in requests created directly
/// from the handler task, as well as from its child tasks. However, some
/// requests need to be completed regardless of whether the initial request
/// timed out, because they are needed for something other than forming the
/// upstream response.
///
/// Deadline propagation is automatically blocked in tasks launched using:
/// @see concurrent::BackgroundTaskStorage::AsyncDetach
/// @see utils::AsyncBackground
///
/// @see concurrent::BackgroundTaskStorage::AsyncDetach does it by default.
class [[nodiscard]] DeadlinePropagationBlocker final {
 public:
  DeadlinePropagationBlocker();

  DeadlinePropagationBlocker(DeadlinePropagationBlocker&&) = delete;
  DeadlinePropagationBlocker& operator=(DeadlinePropagationBlocker&&) = delete;
  ~DeadlinePropagationBlocker();

 private:
  TaskInheritedData old_value_;
};

}  // namespace server::request

USERVER_NAMESPACE_END
