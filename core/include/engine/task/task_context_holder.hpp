#pragma once

/// @file engine/task/task_context_holder.hpp
/// @brief engine::Task implementation hiding helpers

#include <functional>

#include <boost/intrusive_ptr.hpp>

#include <engine/task/task.hpp>

namespace engine {

class TaskProcessor;

namespace impl {

class TaskContext;

using Payload = std::function<void()>;

class TaskContextHolder {
 public:
  ~TaskContextHolder();

  TaskContextHolder(TaskContextHolder&&) noexcept;
  TaskContextHolder& operator=(TaskContextHolder&&) noexcept;

  static TaskContextHolder MakeContext(TaskProcessor&, Task::Importance,
                                       Payload);

  boost::intrusive_ptr<TaskContext> Release();

 private:
  explicit TaskContextHolder(boost::intrusive_ptr<TaskContext>&&);

  boost::intrusive_ptr<TaskContext> context_;
};

}  // namespace impl
}  // namespace engine
