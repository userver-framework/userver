#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <cstddef>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

template <typename T>
class GlobalQueue final {
 public:
  GlobalQueue() = default;

  std::size_t Size() const noexcept { return size_.load(); }

  void Push(T* ctx) {
    size_.fetch_add(1);
    queue_.enqueue(std::move(ctx));
  }

  void PushBulk(utils::span<T*> buffer) {
    size_.fetch_add(buffer.size());
    queue_.enqueue_bulk(buffer.data(), buffer.size());
  }

  std::size_t PopBulk(utils::span<T*> buffer) {
    std::size_t dequed_size = 0;
    while (dequed_size < buffer.size() && size_.load() > 0) {
      std::size_t deq_size = buffer.size() - dequed_size;
      if (deq_size == 0) {
        break;
      }
      std::size_t bulk_size =
          queue_.try_dequeue_bulk(buffer.data() + dequed_size, deq_size);
      size_.fetch_sub(bulk_size);
      dequed_size += bulk_size;
    }
    return dequed_size;
  }

  // Returns nullptr if there are no elements in the queue
  T* TryPop() {
    T* context = nullptr;
    while (size_.load() > 0) {
      if (queue_.try_dequeue(context)) {
        size_.fetch_sub(1);
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
