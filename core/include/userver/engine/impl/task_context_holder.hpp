#pragma once

#include <memory>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t GetTaskContextSize();

/// Helper class to construct both TaskContext and WrappedCall within
/// a single allocation.
/// Although it resides in public includes you should not use it manually
/// (with the engine tests being the single exception).
class TaskContextHolder final {
 public:
  TaskContextHolder(TaskContextHolder&& other) noexcept;
  ~TaskContextHolder();

  template <typename Function, typename... Args>
  static TaskContextHolder Allocate(Function&& f, Args&&... args) {
    using WrappedCallT = utils::impl::WrappedCallImplT<Function, Args...>;

    const auto task_context_size = GetTaskContextSize();
    constexpr auto wrapped_size = sizeof(WrappedCallT);
    constexpr auto wrapped_alignment = alignof(WrappedCallT);
    // check that alignment of WrappedCall is indeed a power of 2,
    // otherwise std::align is UB
    static_assert(
        wrapped_alignment > 0 &&
            (wrapped_alignment & (wrapped_alignment - 1)) == 0,
        "Alignment of Function passed to Async should be a power of 2");
    const auto total_alloc_size =
        task_context_size + wrapped_size + wrapped_alignment;

    std::unique_ptr<std::byte[]> storage{
        static_cast<std::byte*>(::operator new[](total_alloc_size))};

    void* payload_ptr = storage.get() + task_context_size;
    auto payload_space = total_alloc_size - task_context_size;
    std::align(wrapped_alignment, wrapped_size, payload_ptr, payload_space);

    utils::impl::PlacementNewWrapCall(payload_ptr, std::forward<Function>(f),
                                      std::forward<Args>(args)...);

    return TaskContextHolder{
        std::move(storage),
        static_cast<utils::impl::WrappedCallBase*>(payload_ptr)};
  }

  boost::intrusive_ptr<TaskContext> ToContext(
      engine::TaskProcessor& task_processor, Task::Importance importance,
      Task::WaitMode wait_mode, engine::Deadline deadline) &&;

 private:
  TaskContextHolder(std::unique_ptr<std::byte[]> storage,
                    utils::impl::WrappedCallBase* payload);

  std::unique_ptr<std::byte[]> storage_;
  utils::impl::WrappedCallBase* payload_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
