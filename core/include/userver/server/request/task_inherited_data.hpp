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
  const std::string* path;

  /// The method of the request
  const std::string& method;

  /// The time when the request started being handled
  std::chrono::steady_clock::time_point start_time;

  /// The time when there is no use handling the request anymore
  engine::Deadline deadline;
};

inline engine::TaskInheritedVariable<TaskInheritedData> kTaskInheritedData;

}  // namespace server::request

USERVER_NAMESPACE_END
