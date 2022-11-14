#include <userver/engine/impl/task_context_holder.hpp>

#include <utility>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t GetTaskContextSize() { return sizeof(TaskContext); }

TaskContextHolder::TaskContextHolder(TaskContextHolder&& other) noexcept
    : storage_{std::move(other.storage_)},
      payload_{std::exchange(other.payload_, nullptr)} {}

TaskContextHolder::~TaskContextHolder() {
  if (payload_ != nullptr) {
    UASSERT_MSG(false,
                "TaskContextHolder is misused: ToContext was never called");
    payload_->~WrappedCallBase();
  }
}

TaskContextHolder::TaskContextHolder(std::unique_ptr<std::byte[]> storage,
                                     utils::impl::WrappedCallBase* payload)
    : storage_{std::move(storage)}, payload_{payload} {
  UASSERT(payload_);
}

boost::intrusive_ptr<TaskContext> TaskContextHolder::ToContext(
    engine::TaskProcessor& task_processor, Task::Importance importance,
    Task::WaitMode wait_mode, engine::Deadline deadline) && {
  UASSERT_MSG(payload_ != nullptr,
              "ToContext shouldn't be called more than once");

  // if TaskContext ctor throws we destroy WrappedCall in catch, otherwise
  // both storage_ and payload_ are managed by resulting intrusive_ptr
  // (see TaskContext and intrusive_ptr_release interaction).
  utils::FastScopeGuard payload_ptr_guard_{
      [this]() noexcept { payload_ = nullptr; }};

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
