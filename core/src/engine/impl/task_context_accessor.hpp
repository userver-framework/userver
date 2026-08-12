#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/task/task_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// @brief Grants privileged access to the engine::impl::TaskContext owned by a
/// task. Core-internal; external modules should use engine::impl::Detach.
class TaskContextAccessor final {
public:
    /// @brief Returns a shared owning reference to the TaskContext of `task`,
    /// leaving the task valid.
    static boost::intrusive_ptr<TaskContext> GetContext(TaskBase& task) noexcept;

    /// @brief Moves the owning reference to the TaskContext out of `task`,
    /// leaving the task invalid. The task is NOT cancelled or waited for.
    static boost::intrusive_ptr<TaskContext> ExtractContext(TaskBase&& task) noexcept;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
