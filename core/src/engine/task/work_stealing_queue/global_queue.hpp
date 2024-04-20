#pragma once

#include <cstddef>

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

template <typename T>
class GlobalQueue final {
 public:
  GlobalQueue() = default;

  std::size_t GetSize() const noexcept {
    return size_.load(std::memory_order_acquire);
  }

  void Push(T* const ctx) {
    size_.fetch_add(1, std::memory_order_release);
    queue_.enqueue(std::move(ctx));
  }

  void PushBulk(const utils::span<T*> buffer) {
    size_.fetch_add(buffer.size(), std::memory_order_release);
    queue_.enqueue_bulk(buffer.data(), buffer.size());
  }

  std::size_t PopBulk(utils::span<T*> buffer) {
    std::size_t dequed_size = 0;
    while (dequed_size < buffer.size() &&
           size_.load(std::memory_order_acquire) > 0) {
      std::size_t deq_size = buffer.size() - dequed_size;
      if (deq_size == 0) {
        break;
      }
      std::size_t bulk_size =
          queue_.try_dequeue_bulk(buffer.data() + dequed_size, deq_size);
      size_.fetch_sub(bulk_size, std::memory_order_release);
      dequed_size += bulk_size;
    }
    return dequed_size;
  }

  // Returns nullptr if there are no elements in the queue
  T* TryPop() {
    T* context = nullptr;
    while (size_.load(std::memory_order_acquire) > 0) {
      if (queue_.try_dequeue(context)) {
        size_.fetch_sub(1, std::memory_order_release);
        return context;
      }
    }
    return nullptr;
  }

 private:
  moodycamel::ConcurrentQueue<T*> queue_;
  std::atomic<std::size_t> size_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
