#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>

#include <engine/task/task.hpp>
#include <engine/task/task_context_holder.hpp>
#include <utils/wrapped_call.hpp>

namespace engine {

class TaskProcessor;

class TaskCancelledException : public std::exception {
 public:
  TaskCancelledException(Task::CancellationReason reason) : reason_(reason) {}

  Task::CancellationReason Reason() const { return reason_; }

 private:
  const Task::CancellationReason reason_;
};

template <typename T>
class AsyncTask : public Task {
 public:
  AsyncTask() = default;
  AsyncTask(TaskProcessor& task_processor, Importance importance,
            std::shared_ptr<utils::WrappedCall<T>>&& wrapped_call_ptr)
      : Task(impl::TaskContextHolder::MakeContext(
            task_processor, importance,
            [wrapped_call_ptr] { wrapped_call_ptr->Perform(); })),
        wrapped_call_ptr_(std::move(wrapped_call_ptr)) {}

  T Get() {
    assert(wrapped_call_ptr_);
    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(GetCancellationReason());
    }
    return wrapped_call_ptr_->Retrieve();
  }

 private:
  std::shared_ptr<utils::WrappedCall<T>> wrapped_call_ptr_;
};

}  // namespace engine
