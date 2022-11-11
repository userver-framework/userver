#pragma once

#include <memory>

#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t GetTaskContextSize();
size_t GetTaskContextAlignment();

struct TaskContextHolder final {
  std::unique_ptr<char[]> storage;
  utils::impl::WrappedCallBase* payload;

  template <typename Function, typename... Args>
  static TaskContextHolder Allocate(Function&& f, Args&&... args) {
    using WrappedCallT = utils::impl::WrappedCallImplT<Function, Args...>;

    const auto task_context_size = GetTaskContextSize();
    constexpr auto wrapped_size = sizeof(WrappedCallT);
    constexpr auto wrapped_alignment = alignof(WrappedCallT);
    // check that alignment of WrappedCall is indeed a power of 2,
    // otherwise std::align is UB
    static_assert(wrapped_alignment > 0 &&
                  (wrapped_alignment & (wrapped_alignment - 1)) == 0);
    const auto total_alloc_size =
        task_context_size + wrapped_size + wrapped_alignment;

    std::unique_ptr<char[]> storage{
        static_cast<char*>(::operator new[](total_alloc_size))};
    if ((reinterpret_cast<uint64_t>(storage.get()) &
         (GetTaskContextAlignment() - 1)) != 0) {
      abort();
    }

    void* payload_ptr = storage.get() + task_context_size;
    auto payload_space = total_alloc_size - task_context_size;
    std::align(wrapped_alignment, wrapped_size, payload_ptr, payload_space);

    utils::impl::PlacementNewWrappedCall(payload_ptr, std::forward<Function>(f),
                                         std::forward<Args>(args)...);

    return TaskContextHolder{
        std::move(storage),
        static_cast<utils::impl::WrappedCallBase*>(payload_ptr)};
  }

 private:
  TaskContextHolder(std::unique_ptr<char[]> storage,
                    utils::impl::WrappedCallBase* payload)
      : storage{std::move(storage)}, payload{payload} {}
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
