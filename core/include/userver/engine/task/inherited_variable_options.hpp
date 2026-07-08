#pragma once

/// @file userver/engine/task/inherited_variable_options.hpp
/// @brief @copybrief engine::TaskInheritedVariablePriority

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Controls whether a @ref engine::TaskInheritedVariable instance is inherited from the creating task.
enum class TaskInheritedVariablePriority : std::uint8_t {
    /// Default priority for @ref engine::TaskInheritedVariable. Propagates to normal tasks.
    kNormal = 0,
    /// The minimum priority for @ref engine::TaskInheritedVariable instances to propagate to background tasks
    /// (e.g. @ref utils::AsyncBackground).
    kBackground = 1,
    /// The minimum priority for @ref engine::TaskInheritedVariable instances to propagate to no-tracing tasks
    /// (e.g. @ref engine::AsyncNoTracing).
    /// As of now, this priority cannot be assigned to a variable.
    kNoTracing = 2,
    /// Do not inherit any variables.
    kNone = 2,
};

}  // namespace engine

USERVER_NAMESPACE_END
