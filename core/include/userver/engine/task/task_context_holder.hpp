#pragma once

/// @file userver/engine/task/task_context_holder.hpp
/// @brief engine::Task implementation hiding helpers

#include <functional>

#include <boost/intrusive_ptr.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

namespace engine::impl {

class TaskContext;

using Payload = std::function<void()>;

class TaskContextHolder final {
 public:
  ~TaskContextHolder();

  TaskContextHolder(TaskContextHolder&&) noexcept;
  TaskContextHolder& operator=(TaskContextHolder&&) noexcept;

  static TaskContextHolder MakeContext(TaskProcessor&, Task::Importance,
                                       Deadline, Payload);

  boost::intrusive_ptr<TaskContext> Release();

 private:
  explicit TaskContextHolder(boost::intrusive_ptr<TaskContext>&&);

  boost::intrusive_ptr<TaskContext> context_;
};

}  // namespace engine::impl
