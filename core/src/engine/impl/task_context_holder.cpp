#include <userver/engine/impl/task_context_holder.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t GetTaskContextSize() { return sizeof(TaskContext); }

boost::intrusive_ptr<TaskContext> TaskContextHolder::ToContext(
    engine::TaskProcessor& task_processor, Task::Importance importance,
    Task::WaitMode wait_mode, engine::Deadline deadline) && {
  UASSERT(payload_ != nullptr);
  try {
    new (storage_.get()) impl::TaskContext{task_processor, importance,
                                           wait_mode, deadline, *payload_};
  } catch (...) {
    payload_->~WrappedCallBase();
    throw;
  }

  return boost::intrusive_ptr<impl::TaskContext>{
      static_cast<impl::TaskContext*>(static_cast<void*>(storage_.release()))};
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
