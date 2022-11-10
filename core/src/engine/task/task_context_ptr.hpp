#pragma once

/*#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <engine/task/task_context.hpp>

#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContextHolderBase : public boost::intrusive_ref_counter<TaskContextHolderBase> {
 public:
  TaskContextHolderBase(TaskProcessor& tp, Task::Importance ti, Task::WaitMode wm, Deadline d) {
    ::new (context_storage)TaskContext{tp, ti, wm, d, nullptr};
  }
  virtual ~TaskContextHolderBase() {
    GetContext()->~TaskContext();
  }

  TaskContext* GetContext() {
    return reinterpret_cast<TaskContext*>(&context_storage);
  }

  virtual utils::impl::WrappedCallBase* GetPayload() = 0;

 private:
  alignas(alignof (TaskContext)) std::byte context_storage[sizeof (TaskContext)]{};
};

template <typename Function, typename... Args>
class TaskContextHolder final : public TaskContextHolderBase {
 public:
  using WrappedCall = utils::impl::WrappedCallImpl<
                          utils::impl::DecayUnref<Function>,
                          utils::impl::DecayUnref<Args>...>;

  TaskContextHolder(TaskProcessor& tp, Task::Importance ti, Task::WaitMode wm, Deadline d,
                    Function&& f, Args&&... args) : TaskContextHolderBase{tp, ti, wm, d}, exists_{true} {
    ::new (wrapped_call_storage)WrappedCall {
          std::forward<Function>(f),
          std::forward_as_tuple(std::forward<Args>(args)...)};

    GetContext()->SetPayload(GetPayload());
  }

  ~TaskContextHolder() override {
    Reset();
  }

  void Reset() {
    if (exists_) {
      exists_ = false;
      TaskContextHolder::GetPayload()->~WrappedCallBase();
    }
  }

  utils::impl::WrappedCallBase* GetPayload() override {
    return reinterpret_cast<utils::impl::WrappedCallBase*>(&wrapped_call_storage);
  }

 private:
  bool exists_{false};

  alignas(alignof (WrappedCall)) std::byte wrapped_call_storage[sizeof (WrappedCall)]{};
};

using TaskContextPtr = std::unique_ptr<TaskContextHolderBase>;

}

USERVER_NAMESPACE_END*/
