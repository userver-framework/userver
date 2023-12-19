#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class TaskContextHolder final {
 public:
  TaskContextHolder() noexcept = default;

  explicit TaskContextHolder(
      boost::intrusive_ptr<TaskContext>&& context) noexcept;

  static TaskContextHolder Adopt(TaskContext& context) noexcept;

  TaskContextHolder(TaskContextHolder&&) noexcept = default;
  TaskContextHolder& operator=(TaskContextHolder&&) = delete;
  ~TaskContextHolder();

  boost::intrusive_ptr<TaskContext>&& Extract() && noexcept;

 private:
  boost::intrusive_ptr<TaskContext> context_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
