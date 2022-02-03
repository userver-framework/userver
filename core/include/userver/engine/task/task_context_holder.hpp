#pragma once

/// @file userver/engine/task/task_context_holder.hpp
/// @brief engine::Task implementation hiding helpers

#include <functional>
#include <memory>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/impl/wrapped_call_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

using Payload = std::shared_ptr<utils::impl::WrappedCallBase>;

class TaskContextHolder final {
 public:
  ~TaskContextHolder();

  TaskContextHolder(TaskContextHolder&&) noexcept;
  TaskContextHolder& operator=(TaskContextHolder&&) noexcept;

  static TaskContextHolder MakeContext(TaskProcessor&, Task::Importance,
                                       Task::WaitMode, Deadline, Payload&&);

  boost::intrusive_ptr<TaskContext> Release();

 private:
  explicit TaskContextHolder(boost::intrusive_ptr<TaskContext>&&);

  boost::intrusive_ptr<TaskContext> context_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
